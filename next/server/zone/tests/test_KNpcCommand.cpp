// The do_skill command of the JX2 server (jx_linux_y: NpcSkillCommand 0x080DD130, KNpc::SendCommand
// 0x0809B750, the check 0x0809B840, KNpc::ProcessCommand 0x0809B9E0, KNpc::CastSkill 0x08088350
// with KSkill::CanCastSkill 0x080E8AE0, KNpc::DoSkill 0x08088150 and the fire at 60 % 0x08085020)
// - docs/LINUX-SERVER.md §16.  Every number here is worked out by hand from the binary.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "jx/log.hpp"
#include "jx/msg.pb.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KMapData.h"
#include "jx/zone/KScriptCache.h"
#include "jx/zone/KSkill.h"
#include "jx/zone/KSkillList.h"
#include "jx/zone/KSubWorld.h"

using namespace jx::zone;
using jx::EntityId;

namespace {

struct Quiet {
    Quiet()
    {
        jx::log::Options o;
        o.console = false;
        o.default_level = jx::log::Level::error;
        jx::log::init(o);
    }
};

// the level script: every test skill takes 5 life at once (an immediate attribute)
constexpr const char* kLevelScript = R"lua(
function GetSkillLevelData(levelname, data, level)
    if data == "hit" and levelname == "life_v" then return "-5,0,0" end
    if data == "buff" and levelname == "armordefense_v" then return "10,60,0" end
    if data == "buff" and levelname == "hide" then return "1,60,0" end
    return ""
end
)lua";

std::string make_scripts()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jxnext_npccommand_test";
    std::filesystem::create_directories(root / "script" / "skill");
    std::ofstream(root / "script" / "skill" / "cmd.lua") << kLevelScript;
    return root.generic_string();
}

// 1 the basic attack (a WeaponSkill row); 1101 a blow on an enemy within 100 for 10 mana with a
// cool down of 30; 1102 a buff on oneself usable out of fight mode; 1103 an aura; 1104 a weapon
// skill of another weapon; 1105 with an EqtLimit; 1106 a blow that reaches 40; 1107 needs level 10
std::shared_ptr<const KSkillTable> skill_table()
{
    KSkillTable t;
    int row = 2;
    auto add = [&](int id, std::vector<std::pair<std::string, std::string>> extra) {
        std::unordered_map<std::string, std::string> cells{{"SkillId", std::to_string(id)}, {"SkillStyle", "2"}, {"Series", "-1"}, {"DoHurt", "0"},
                                                           {"IsPhysical", "1"}, {"TargetEnemy", "1"}, {"AttackRadius", "100"}, {"EqtLimit", "-2"},
                                                           {"LvlSetScript", "\\script\\skill\\cmd.lua"}, {"LvlSetting1", "life_v"}, {"LvlData1", "hit"},
                                                           {"MaxLevel", "20"}, {"ReqLevel", "1"}, {"CharAnimId", "9"}};
        for (auto& [k, v] : extra) cells[k] = v;
        KSkillRow r = KSkillRow::from_cells(cells);
        r.row = row++;
        r.max_level = 20;
        t.add(r);
    };
    add(1, {{"WeaponSkill", "1"}});
    add(1101, {{"CostValue", "10"}, {"SkillCostType", "0"}, {"TimePerCast", "30"}});
    add(1102, {{"TargetEnemy", "0"}, {"TargetSelf", "1"}, {"PeaceCanUse", "1"}, {"IsPhysical", "0"}, {"LvlSetting1", "armordefense_v"}, {"LvlData1", "buff"}});
    add(1103, {{"IsAura", "1"}});
    add(1104, {{"WeaponSkill", "1"}});
    add(1105, {{"EqtLimit", "3"}});
    add(1106, {{"AttackRadius", "40"}, {"SkillStyle", "0"}, {"MisslesForm", "3"}, {"Param1", "0"}});   // CanCastSkill checks no reach for it (form 3, Param1 != 1): ProcessCommand walks up
    add(1107, {{"ReqLevel", "10"}});
    add(1108, {{"TargetEnemy", "0"}, {"TargetSelf", "1"}, {"PeaceCanUse", "1"}, {"IsPhysical", "0"}, {"LvlSetting1", "hide"}, {"LvlData1", "buff"}});
    t.set_attrib_data(magic_hide, {70, 713, 1108});   // [hide] of attribconstdata.ini: Data0 the transparency, Data1.. the state skills
    // the moves of style 1 (MisslesForm 8..13): 1109 a jump to a spot, 1110 a jump at the target then the child blow 1106,
    // 1111 a run at the target with +5 speed then 1106, 1112 the child 1106 three times, 1113 a blink within 300 after
    // 6 frames, 1114 the child 1101 (any style) at 60 % of an attack action
    add(1109, {{"SkillStyle", "1"}, {"MisslesForm", "9"}, {"TargetEnemy", "0"}, {"TimePerCast", "30"}});
    add(1110, {{"SkillStyle", "1"}, {"MisslesForm", "10"}, {"ChildSkillId", "1106"}});
    add(1111, {{"SkillStyle", "1"}, {"MisslesForm", "11"}, {"ChildSkillId", "1106"}, {"Param1", "5"}, {"WaitTime", "20"}});   // the run lasts WaitTime frames at most (0x080E8650(sk, 0))
    add(1112, {{"SkillStyle", "1"}, {"MisslesForm", "12"}, {"ChildSkillId", "1106"}, {"ChildSkillNum", "3"}});
    add(1113, {{"SkillStyle", "1"}, {"MisslesForm", "13"}, {"TargetEnemy", "0"}, {"Param1", "300"}, {"Param2", "6"}});
    add(1114, {{"SkillStyle", "1"}, {"MisslesForm", "8"}, {"ChildSkillId", "1101"}});
    return std::make_shared<const KSkillTable>(std::move(t));
}

KSubWorldConfig small_world()
{
    KSubWorldConfig c;
    c.zone_id = 1;
    c.tick_hz = 18;
    c.width = 4096;
    c.height = 4096;
    c.cell_size = 512;
    c.spawn_point = Pos{2000, 2000};
    c.default_speed = 200;
    c.map_npcs = false;
    c.skills = skill_table();
    c.scripts = std::make_shared<KScriptCache>(make_scripts());
    return c;
}

jx::pb::RoleData role(std::uint64_t pid, const std::string& name, Pos at, std::vector<int> skills)
{
    jx::pb::RoleData r;
    r.set_player_id(pid);
    r.set_name(name);
    r.set_level(5);
    r.mutable_stats()->set_strength(100);
    r.mutable_stats()->set_dexterity(100);
    r.mutable_stats()->set_vitality(50);
    r.mutable_stats()->set_energy(10);
    r.mutable_stats()->set_hp_max(500);
    r.mutable_stats()->set_hp(500);
    r.mutable_stats()->set_mp_max(50);
    r.mutable_stats()->set_mp(50);
    r.set_fight_mode(true);
    r.mutable_position()->set_zone_id(1);
    r.mutable_position()->mutable_pos()->set_x(at.x);
    r.mutable_position()->mutable_pos()->set_y(at.y);
    for (int id : skills) {
        jx::pb::RoleSkill* s = r.add_skills();
        s->set_id(static_cast<std::uint32_t>(id));
        s->set_level(1);
    }
    return r;
}

struct Arena {
    Quiet quiet;
    KSubWorld w;
    EntityId hero;
    EntityId pig;
    KNpc* h = nullptr;
    KNpc* p = nullptr;
    explicit Arena(std::vector<int> skills = {1, 1101, 1102, 1103, 1104, 1105, 1106, 1107}, Pos pig_at = Pos{2050, 2000})
        : w(small_world())
    {
        Pos at;
        REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}, std::move(skills)), hero, at) == jx::pb::RESULT_OK);
        pig = w.spawn_npc("pig", pig_at, 418, 0, KNpcKind::monster);
        h = w.mutable_entity(hero);
        p = w.mutable_entity(pig);
        REQUIRE(h != nullptr);
        REQUIRE(p != nullptr);
        p->base.life_max = 1000;
        p->cur.life_max = p->cur.life_max_yan = 1000;
        p->cur.life = 1000;
        p->cur.defend = 0;
        p->camp = p->current_camp = camp_animal;
        p->cur.physics_resist_max = 100;
        w.take_outbox();
    }
    std::vector<jx::pb::EntityAction> actions()
    {
        std::vector<jx::pb::EntityAction> out;
        for (const Packet& pk : w.take_outbox()) {
            if (pk.msg_id != static_cast<std::uint16_t>(jx::pb::G2C_ENTITY_ACTION)) continue;
            jx::pb::EntityAction a;
            REQUIRE(a.ParseFromString(pk.payload));
            if (a.entity_id() == hero.value && a.action() == jx::pb::ACTION_ATTACK) out.push_back(a);
        }
        return out;
    }
    void ticks(int n)
    {
        for (int i = 0; i < n; ++i) w.tick();
    }
};

} // namespace

TEST_CASE("SendCommand: only a skill the list holds, five at most, a command ages away", "[command]")
{
    Arena a({1101});
    // 0x0809B75E: FindSame first
    CHECK_FALSE(a.w.send_command(*a.h, 1102, -1, 0, a.pig));
    CHECK(a.h->commands.empty());
    // the ring of five (+0x171c when full)
    for (int i = 0; i < 5; ++i) CHECK(a.w.send_command(*a.h, 1101, -1, 0, a.pig));
    CHECK_FALSE(a.w.send_command(*a.h, 1101, -1, 0, a.pig));
    CHECK(a.h->commands.size() == 5);
    CHECK(a.h->commands.front().life == KNpc::kCommandLife);
    // 0x0809B510: while the npc is busy every command ages a frame and goes at 0
    a.h->doing = KDoing::attack;
    a.h->frame_total = 1000;
    a.h->frame_cur = 0;
    a.ticks(KNpc::kCommandLife - 1);
    CHECK(a.h->commands.size() == 5);
    a.ticks(1);
    CHECK(a.h->commands.empty());
}

TEST_CASE("the cast request: what the skill handler and CanCastSkill refuse", "[command]")
{
    Arena a;
    // 0x080DD130: an aura is switched, not cast; a bad id; a spot below zero; a target that is not there
    CHECK_FALSE(a.w.cast_skill_request(7, 1103, -1, 0, a.pig, 1));
    CHECK_FALSE(a.w.cast_skill_request(7, 2000, -1, 0, a.pig, 2));
    CHECK_FALSE(a.w.cast_skill_request(7, 1101, -5, 10, EntityId{}, 3));
    CHECK_FALSE(a.w.cast_skill_request(7, 1101, -1, 0, EntityId{9999}, 4));
    CHECK(a.actions().empty());
    // 0x080E8B5E: TargetEnemy on oneself is no target; an enemy is
    CHECK(a.w.cast_skill_request(7, 1101, -1, 0, a.hero, 5));   // queued and dropped by the check
    CHECK(a.actions().empty());
    CHECK(a.h->doing == KDoing::stand);
    // a WeaponSkill row must be the weapon's own skill (0x08079A90: empty hands -> the basic attack 1)
    CHECK(a.w.cast_skill_request(7, 1104, -1, 0, a.pig, 6));
    CHECK(a.actions().empty());
    // EqtLimit 3 against empty hands (-1)
    CHECK(a.w.cast_skill_request(7, 1105, -1, 0, a.pig, 7));
    CHECK(a.actions().empty());
    // ReqLevel 10 at level 5: the list's CanCast in CastSkill (0x08088429) refuses
    CHECK(a.w.cast_skill_request(7, 1107, -1, 0, a.pig, 8));
    CHECK(a.actions().empty());
    CHECK(a.h->commands.empty());
    // the basic attack itself goes
    CHECK(a.w.cast_skill_request(7, 1, -1, 0, a.pig, 9));
    auto acts = a.actions();
    REQUIRE(acts.size() == 1);
    CHECK(acts[0].skill_id() == 1);
    CHECK(acts[0].skill_level() == 1);
    CHECK(acts[0].target() == a.pig.value);
}

TEST_CASE("CastSkill: the action, the cost, the fire at 60 percent, the cool down", "[command]")
{
    Arena a;
    CHECK(a.w.cast_skill_request(7, 1101, -1, 0, a.pig, 1));
    auto acts = a.actions();
    REQUIRE(acts.size() == 1);
    CHECK(acts[0].skill_id() == 1101);
    CHECK(acts[0].frames() == 18);   // AttackFrame 18 x 100 / (attack speed 0 + 100)
    CHECK(acts[0].dir() == 48);      // the pig to the right
    CHECK(a.h->doing == KDoing::attack);
    CHECK(a.h->active_skill_id == 1101);
    CHECK(a.h->cur.attack_radius == 100);   // SetActiveSkill 0x08086D90: +0x12a8 from the skill
    CHECK(a.h->cur.mana == 40);             // 0x08078B10: 10 mana paid at CastSkill
    // nothing lands before 60 % of the frames (18 x 60 / 100 = 10), the blow at the 10th
    a.ticks(9);
    CHECK(a.p->life() == 1000);
    a.ticks(1);
    CHECK(a.p->life() == 995);
    // 0x080847B0: the cool down (TimePerCast 30) starts at the fire
    CHECK(a.h->skill_list.next_cast_time(1101) == a.w.tick_count() + 30);
    CHECK(a.h->skill_list.cool_down_time(1101) == 30);
    // the action ends after its 18 frames, the character stands
    a.ticks(9);
    CHECK(a.h->doing == KDoing::stand);
    // a second cast within the cool down is dropped, after it goes again (the zone re-sends the
    // command by itself while the target lives: the old client re-sent it each swing)
    a.h->attack_target = EntityId{};
    CHECK(a.w.cast_skill_request(7, 1101, -1, 0, a.pig, 2));
    CHECK(a.actions().empty());
    a.h->attack_target = EntityId{};
    a.ticks(22);   // the cool down started at the fire (tick 10): over at tick 40
    a.actions();
    CHECK(a.w.cast_skill_request(7, 1101, -1, 0, a.pig, 3));
    CHECK(a.actions().size() == 1);
    CHECK(a.h->cur.mana == 30);
    // a skill the mana cannot pay for is refused
    a.h->attack_target = EntityId{};
    a.h->cur.mana = 5;
    a.ticks(30);
    a.actions();
    CHECK(a.w.cast_skill_request(7, 1101, -1, 0, a.pig, 4));
    CHECK(a.actions().empty());
    CHECK(a.h->cur.mana == 5);
}

TEST_CASE("PeaceCanUse out of fight mode, a self skill at a spot casts on oneself, do_magic frames", "[command]")
{
    Arena a;
    a.h->fight_mode = false;
    // 0x0809B8B2: out of fight mode a skill without PeaceCanUse is dropped
    CHECK(a.w.cast_skill_request(7, 1101, -1, 0, a.pig, 1));
    CHECK(a.actions().empty());
    // 0x080E8B33: a TargetSelf skill cast at a spot is cast on oneself; a magic skill takes CastFrame
    CHECK(a.w.cast_skill_request(7, 1102, 2100, 2100, EntityId{}, 2));
    auto acts = a.actions();
    REQUIRE(acts.size() == 1);
    CHECK(acts[0].skill_id() == 1102);
    CHECK(acts[0].target() == a.hero.value);
    CHECK(acts[0].frames() == 18);   // CastFrame 18
    CHECK(a.h->doing == KDoing::magic);
    const int defend = a.h->cur.defend;
    CHECK(a.h->active_skill_id == 1102);
    CHECK(a.h->cast_param1 == -1);
    CHECK(a.h->cast_target == a.hero);
    a.ticks(10);
    CHECK(a.h->skill_list.next_cast_time(1102) > 0);   // the fire reached the cool down
    CHECK(a.h->cur.defend == defend + 10);   // the buff (defense_v 10 for 60 frames) landed on oneself
    CHECK(a.h->state_of(1102) != nullptr);
}

TEST_CASE("ProcessCommand: walk up within 300 of the reach, drop beyond", "[command]")
{
    Arena near({1106}, Pos{2050, 2000});
    // 40 of reach, the pig at 50: the command waits while the character walks up
    CHECK(near.w.cast_skill_request(7, 1106, -1, 0, near.pig, 1));
    CHECK(near.actions().empty());
    CHECK(near.h->commands.size() == 1);
    CHECK(near.h->moving);
    bool cast = false;
    for (int i = 0; i < 40 && !cast; ++i) {
        near.w.tick();
        cast = !near.actions().empty();
    }
    CHECK(cast);
    CHECK(near.h->commands.empty());
    // 400 away: beyond reach + 300, dropped at once (0x0809BB07)
    Arena far({1106}, Pos{2400, 2000});
    CHECK(far.w.cast_skill_request(7, 1106, -1, 0, far.pig, 1));
    CHECK(far.h->commands.empty());
    CHECK_FALSE(far.h->moving);
    CHECK(far.actions().empty());
}

TEST_CASE("the old attack request is the weapon's physical skill and keeps striking", "[command]")
{
    Arena a({1});
    REQUIRE(a.w.attack_request(7, a.pig, 1));
    auto acts = a.actions();
    REQUIRE(acts.size() == 1);
    CHECK(acts[0].skill_id() == 1);
    int swings = 1;
    for (int i = 0; i < 60; ++i) {
        a.w.tick();
        swings += static_cast<int>(a.actions().size());
    }
    CHECK(swings >= 3);
    CHECK(a.p->life() < 1000);
}

// KNpc::SetHide 0x0807FF80, KNpc::IsInvisibleTo 0x08079200, the hiding broken 0x0807D4C0, the
// ProcessFunc of [hide] 200 0x08097860 - docs/LINUX-SERVER.md §16.1
TEST_CASE("hide: the players around forget the npc, its own client keeps it, the cast breaks it", "[command]")
{
    Arena a({1, 1101, 1102, 1108});
    EntityId watcher;
    Pos at;
    REQUIRE(a.w.spawn_player(8, role(80, "Watcher", Pos{2100, 2000}, {1}), watcher, at) == jx::pb::RESULT_OK);
    a.h = a.w.mutable_entity(a.hero);   // the entity table may have moved
    a.p = a.w.mutable_entity(a.pig);
    a.ticks(2);
    a.w.take_outbox();
    KNpc* wn = a.w.mutable_entity(watcher);
    REQUIRE(wn != nullptr);
    REQUIRE(std::binary_search(a.h->watchers.begin(), a.h->watchers.end(), 8u));
    const auto packets_to = [&](std::uint64_t sid, jx::pb::MsgId id, std::uint64_t entity) {
        int n = 0;
        for (const Packet& pk : a.w.take_outbox()) {
            if (pk.msg_id != static_cast<std::uint16_t>(id) || std::find(pk.sids.begin(), pk.sids.end(), sid) == pk.sids.end()) continue;
            if (id == jx::pb::G2C_ENTITY_DESPAWN) {
                jx::pb::EntityDespawn d;
                REQUIRE(d.ParseFromString(pk.payload));
                for (const auto e : d.entity_ids()) n += e == entity;
            } else {
                jx::pb::EntitySpawn s;
                REQUIRE(s.ParseFromString(pk.payload));
                for (const auto& e : s.entities()) n += e.entity_id() == entity && e.hide() == 0;
            }
        }
        return n;
    };
    // the hide buff on oneself ([hide] 1 for 60 frames) fires at frame 10 -> KNpc::SetHide(1): the
    // watcher's client is sent the despawn (the 0x4f packet), the hero's own client keeps its npc
    CHECK(a.w.cast_skill_request(7, 1108, -1, 0, a.hero, 1));
    a.ticks(11);
    CHECK(a.h->hide == 1);
    CHECK(a.h->state_of(1108) != nullptr);
    CHECK(a.w.invisible_to(*a.h, watcher));
    CHECK_FALSE(a.w.invisible_to(*a.h, a.hero));
    CHECK_FALSE(a.w.invisible_to(*wn, a.hero));
    CHECK(a.h->watchers == std::vector<std::uint64_t>{7});
    CHECK(packets_to(8, jx::pb::G2C_ENTITY_DESPAWN, a.hero.value) == 1);
    // the watcher keeps looking around and does not learn it again while it is hidden
    a.ticks(10);
    CHECK(a.h->watchers == std::vector<std::uint64_t>{7});
    CHECK(packets_to(8, jx::pb::G2C_ENTITY_SPAWN, a.hero.value) == 0);
    // 0x080884A3: a cast breaks the hiding - the state skill named by [hide] Data2 (1108) comes off,
    // its negated [hide] runs KNpc::SetHide(0); the watcher looks again at once and learns the hero
    CHECK(a.w.cast_skill_request(7, 1101, -1, 0, a.pig, 2));
    CHECK(a.h->hide == 0);
    CHECK(a.h->state_of(1108) == nullptr);
    CHECK_FALSE(a.w.invisible_to(*a.h, watcher));
    a.ticks(1);
    CHECK(std::binary_search(a.h->watchers.begin(), a.h->watchers.end(), 8u));
    CHECK(packets_to(8, jx::pb::G2C_ENTITY_SPAWN, a.hero.value) == 1);
}

TEST_CASE("hide on a npc: SetHide, a removal off an unhidden npc changes nothing, the death breaks it", "[command]")
{
    Arena a;
    a.ticks(4);   // the hero's next look: it learns the pig (spawned after its first look)
    REQUIRE(a.p->watchers == std::vector<std::uint64_t>{7});
    // KNpc::SetHide(1): the hero's client forgets the pig
    a.w.set_hide(*a.p, 1);
    CHECK(a.p->hide == 1);
    CHECK(a.p->watchers.empty());
    CHECK(a.w.invisible_to(*a.p, a.hero));
    a.ticks(8);
    CHECK(a.p->watchers.empty());
    // KNpc::SetHide(0): the viewers around look again and learn it
    a.w.set_hide(*a.p, 0);
    CHECK(a.p->hide == 0);
    a.ticks(1);
    CHECK(a.p->watchers == std::vector<std::uint64_t>{7});
    // 0x08097860: a [hide] taken off a npc that is not hidden leaves 0 (the value would go negative)
    KMagicAttrib off;
    off.type = magic_hide;
    off.value = {-1, 0, 0};
    a.w.modify_attrib(*a.p, a.pig, off, true);
    CHECK(a.p->hide == 0);
    // the state itself on the pig, then its death: 0x08089359 breaks the hiding
    KMagicAttrib on;
    on.type = magic_hide;
    on.value = {1, 60, 0};
    CHECK(a.w.set_state_skill_effect(*a.p, a.hero, 1108, 1, &on, 1, 60) == 0);
    CHECK(a.p->hide == 1);
    CHECK(a.p->watchers.empty());
    a.p->cur.sorb_damage = 0;   // a blow of 3 on 3 life kills it: KNpc::DoDeath 0x080892E0
    a.p->cur.life = 3;
    int dealt = 0;
    CHECK(a.w.calc_damage(*a.p, *a.h, 3, 3, damage_physics, true, nullptr, &dealt, 0, false) == 0);
    CHECK(a.p->doing == KDoing::death);
    CHECK(a.p->hide == 0);
    CHECK(a.p->state_of(1108) == nullptr);
}

// The moves of style 1: KNpc 0x08087F70 (the jump table 0x08254AC0 of MisslesForm 8..13), the jump
// 0x08087CF0 / 0x0807B320 / 0x080818F0, the forms 0x08084930 / 0x080807E0 / 0x08084A10 / 0x08084B40 /
// 0x08084C90 and their frames - docs/LINUX-SERVER.md §16.2.  Every number is worked out from the binary.
namespace {
int jumps_of(Arena& a, std::uint64_t entity, Pos& aim, std::uint32_t& frames)
{
    int n = 0;
    for (const Packet& pk : a.w.take_outbox()) {
        if (pk.msg_id != static_cast<std::uint16_t>(jx::pb::G2C_ENTITY_ACTION)) continue;
        jx::pb::EntityAction act;
        REQUIRE(act.ParseFromString(pk.payload));
        if (act.entity_id() != entity || act.action() != jx::pb::ACTION_JUMP) continue;
        ++n;
        aim = Pos{act.aim().x(), act.aim().y()};
        frames = act.frames();
    }
    return n;
}
} // namespace

TEST_CASE("style 1: a jump lands 40 steps away at most, shorter than 20 is none, a blink teleports", "[command]")
{
    Arena a({1109, 1113});
    // form 9 at a spot 200 away: the way is free, 200 / 12 = 16 steps of 12 -> 16 frames in the air,
    // the landing spot (2192, 2000), the direction east (48), the 0x54 packet = ACTION_JUMP with the spot
    CHECK(a.w.cast_skill_request(7, 1109, 2200, 2000, EntityId{}, 1));
    CHECK(a.h->doing == KDoing::jump);
    CHECK(a.h->frame_total == 16);
    CHECK(a.h->jump_steps == 16);
    CHECK(a.h->jump_arc == 75);
    CHECK(a.h->knock_dest == Pos{2192, 2000});
    CHECK(a.h->dir == 48);
    CHECK(a.h->skill_list.next_cast_time(1109) == 30);   // the cool down (TimePerCast 30) taken as the move starts (0x08087FED)
    Pos aim;
    std::uint32_t frames = 0;
    CHECK(jumps_of(a, a.hero.value, aim, frames) == 1);
    CHECK(aim == Pos{2192, 2000});
    CHECK(frames == 16);
    // 0x080817E0: 12 a frame, the height curve 5 f (15 - f) / 8 - 0 at both ends, 26 in the middle
    a.ticks(1);
    CHECK(a.h->pos() == Pos{2012, 2000});
    CHECK(a.h->height == 0);
    a.ticks(7);
    CHECK(a.h->pos() == Pos{2096, 2000});
    CHECK(a.h->height == 5 * 7 * 8 / 8);
    a.ticks(8);
    CHECK(a.h->pos() == Pos{2192, 2000});
    CHECK(a.h->doing == KDoing::stand);
    CHECK(a.h->height == 0);
    // 0x08087D02: 40 x 12 = 480 is as far as a jump goes
    a.h->skill_list.clear_cool_time(0);
    CHECK(a.w.cast_skill_request(7, 1109, 3000, 2000, EntityId{}, 2));
    CHECK(a.h->doing == KDoing::jump);
    CHECK(a.h->frame_total == 40);
    CHECK(a.h->knock_dest == Pos{2672, 2000});
    a.ticks(40);
    CHECK(a.h->pos() == Pos{2672, 2000});
    // 0x08087D3E: a way of 20 or less is no jump - the skill fails and the character stands
    a.h->skill_list.clear_cool_time(0);
    CHECK(a.w.cast_skill_request(7, 1109, 2680, 2000, EntityId{}, 3));
    CHECK(a.h->doing == KDoing::stand);
    CHECK(a.h->pos() == Pos{2672, 2000});
    // form 13: a blink within Param1 = 300 after Param2 = 6 frames (0x08084C90 / 0x08080760)
    CHECK(a.w.cast_skill_request(7, 1113, 2672, 2500, EntityId{}, 4));
    CHECK(a.h->doing == KDoing::blink);
    CHECK(a.h->frame_total == 6);
    CHECK(a.h->knock_dest == Pos{2672, 2300});
    a.ticks(5);
    CHECK(a.h->pos() == Pos{2672, 2000});
    a.ticks(1);
    CHECK(a.h->pos() == Pos{2672, 2300});
    CHECK(a.h->doing == KDoing::stand);
    // a form outside 8..13 of a style-1 skill is refused (0x08087F8E): none in the table here
}

TEST_CASE("style 1: the jump attack lands by the target and strikes, the run attack runs there with the bonus", "[command]")
{
    Arena a({1110, 1111});
    // form 10 at the pig (2050): the spot x + 1 = 2051 is 51 away -> 4 steps, landing at 2048; then the
    // strike of 18 attack frames whose child blow (style 0) flies at 60 % and ends the move at once
    CHECK(a.w.cast_skill_request(7, 1110, -1, 0, a.pig, 1));
    CHECK(a.h->doing == KDoing::jump_attack);
    CHECK(a.h->phase == 0);
    CHECK(a.h->frame_total == 4);
    CHECK(a.h->knock_dest == Pos{2048, 2000});
    CHECK(a.h->cast_kept1 == -1);
    CHECK(a.h->cast_kept_target == a.pig);
    Pos aim;
    std::uint32_t frames = 0;
    CHECK(jumps_of(a, a.hero.value, aim, frames) == 1);
    a.ticks(4);
    CHECK(a.h->pos() == Pos{2048, 2000});
    CHECK(a.h->doing == KDoing::jump_attack);
    CHECK(a.h->phase == 1);
    CHECK(a.h->frame_total == 18);
    REQUIRE(a.actions().size() == 1);   // the strike (ACTION_ATTACK with the skill)
    a.ticks(9);
    CHECK(a.h->doing == KDoing::jump_attack);
    a.ticks(1);   // frame 10 = 60 %: the child blow, then DoStand (0x08084F20)
    CHECK(a.h->doing == KDoing::stand);
    CHECK(a.h->phase == 0);
    CHECK(a.h->attack_target.value == 0);   // no self-repeat of a move
    // form 11: a run at the pig with Param1 = 5 more a frame (90 more a second at 18 Hz); the child
    // blow when it arrives (or after WaitTime 20 frames), then it stands and the bonus comes off (0x080853B0)
    a.h->set_pos(Pos{2000, 2000});
    const std::uint32_t speed = a.h->speed;
    CHECK(a.w.cast_skill_request(7, 1111, -1, 0, a.pig, 2));
    CHECK(a.h->doing == KDoing::run);
    CHECK(a.h->moving);
    CHECK(a.h->run_bonus == 5);
    CHECK(a.h->speed == speed + 90);
    CHECK(a.h->knock_dest == Pos{2050, 2000});
    int ticks = 0;
    while (a.h->doing == KDoing::run && ticks < 60) {
        a.w.tick();
        ++ticks;
    }
    CHECK(a.h->doing == KDoing::stand);
    CHECK(a.h->run_bonus == 0);
    CHECK(a.h->speed == speed);
    CHECK_FALSE(a.h->moving);
    CHECK(ticks >= 2);
    CHECK(ticks <= 6);
    CHECK(a.h->pos() == Pos{2050, 2000});
}

TEST_CASE("style 1: the multi cast fires the child ChildSkillNum times, the special attack casts it at 60 percent", "[command]")
{
    Arena a({1112, 1114});
    // form 12: three casts, each after the start delay of the i-th missile (none here: 1 frame each)
    CHECK(a.w.cast_skill_request(7, 1112, -1, 0, a.pig, 1));
    CHECK(a.h->doing == KDoing::special_cast);
    CHECK(a.h->phase == 0);
    CHECK(a.h->frame_total == 1);
    CHECK(a.actions().size() == 1);
    a.ticks(1);
    CHECK(a.h->doing == KDoing::special_cast);
    CHECK(a.h->phase == 1);
    a.ticks(1);
    CHECK(a.h->phase == 2);
    a.ticks(1);   // the third cast, then 0x08084B61: past ChildSkillNum the npc stands
    CHECK(a.h->doing == KDoing::stand);
    CHECK(a.h->phase == 0);
    CHECK(a.actions().size() == 2);
    // form 8: an attack action of 18 frames; the child (1101, an immediate blow of 5 life) at frame 10
    CHECK(a.w.cast_skill_request(7, 1114, -1, 0, a.pig, 2));
    CHECK(a.h->doing == KDoing::special_skill);
    CHECK(a.h->frame_total == 18);
    a.ticks(9);
    CHECK(a.p->life() == 1000);
    a.ticks(1);
    CHECK(a.p->life() == 995);
    CHECK(a.h->doing == KDoing::special_skill);
    a.ticks(8);
    CHECK(a.h->doing == KDoing::stand);
    CHECK(a.p->life() == 995);
    // a move is an action: another command waits (0x0809B840 answers 1 while +0x194c == 0)
    a.h->skill_list.clear_cool_time(0);
    CHECK(a.w.cast_skill_request(7, 1114, -1, 0, a.pig, 3));
    CHECK(a.h->doing == KDoing::special_skill);
    CHECK(a.w.cast_skill_request(7, 1112, -1, 0, a.pig, 4));
    CHECK(a.h->commands.size() == 1);
    CHECK(a.h->doing == KDoing::special_skill);
}
