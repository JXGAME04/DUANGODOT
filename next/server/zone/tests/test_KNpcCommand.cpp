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
#include "jx/client.pb.h"
#include "jx/msg.pb.h"
#include "jx/zone/KFaction.h"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KMapData.h"
#include "jx/zone/KNpcTemplate.h"
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
    if data == "summon" and levelname == "createnpc" then return "418,5,0" end
    if data == "combo" and levelname == "skill_appendskill" then return "1101,3,0" end
    if data == "combo" and levelname == "skill_showevent" then return "2,0,0" end
    if data == "combo" and levelname == "addskilldamage1" then return "1101,0,25" end
    if data == "unheld" and levelname == "skill_appendskill" then return "1108,1,0" end
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
    add(1103, {{"IsAura", "1"}, {"ChildSkillId", "1102"}, {"StateSpecialId", "45"}, {"StatePriority", "2"}});   // an aura whose child is the buff 1102
    add(1104, {{"WeaponSkill", "1"}});
    add(1105, {{"EqtLimit", "3"}});
    add(1106, {{"AttackRadius", "40"}, {"SkillStyle", "0"}, {"MisslesForm", "3"}, {"Param1", "0"}});   // CanCastSkill checks no reach for it (form 3, Param1 != 1): ProcessCommand walks up
    add(1107, {{"ReqLevel", "10"}});
    add(1108, {{"TargetEnemy", "0"}, {"TargetSelf", "1"}, {"PeaceCanUse", "1"}, {"IsPhysical", "0"}, {"LvlSetting1", "hide"}, {"LvlData1", "buff"}});
    add(1130, {{"SkillStyle", "3"}, {"TargetEnemy", "0"}, {"TargetSelf", "1"}, {"IsPhysical", "0"}, {"LvlSetting1", "armordefense_v"}, {"LvlData1", "buff"}});   // a passive (style 3) for a template's PasstSkillId
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
    // 1115..1117 create-npc skills (style 4) of kinds 2, 3 and 4: one npc of template 418 at level 5 at the spot
    add(1115, {{"SkillStyle", "4"}, {"TargetEnemy", "0"}, {"Param1", "2"}, {"ChildSkillNum", "1"}, {"Series", "0"}, {"LvlSetting1", "createnpc"}, {"LvlData1", "summon"}});
    add(1116, {{"SkillStyle", "4"}, {"TargetEnemy", "0"}, {"Param1", "3"}, {"ChildSkillNum", "1"}, {"Series", "0"}, {"LvlSetting1", "createnpc"}, {"LvlData1", "summon"}});
    add(1117, {{"SkillStyle", "4"}, {"TargetEnemy", "0"}, {"Param1", "4"}, {"ChildSkillNum", "1"}, {"Series", "0"}, {"LvlSetting1", "createnpc"}, {"LvlData1", "summon"}});
    // 1120 names skills in its tip: the append skill 1101 (at most level 3), the fly event skill 1102 (ShowEvent bit 2 from the
    // level script, EventSkillLevel -1 = its own level), and its addskilldamage1 puts 25 on 1101; 1121 names 1108 (not held)
    add(1120, {{"LvlSetting1", "life_v"}, {"LvlData1", "hit"}, {"LvlSetting2", "skill_appendskill"}, {"LvlData2", "combo"}, {"LvlSetting3", "skill_showevent"},
               {"LvlData3", "combo"}, {"LvlSetting4", "addskilldamage1"}, {"LvlData4", "combo"}, {"FlySkillId", "1102"}, {"EventSkillLevel", "-1"}});
    add(1121, {{"LvlSetting2", "skill_appendskill"}, {"LvlData2", "unheld"}});
    // 1118 only on foot (HorseLimit 1), 1119 only on a horse (HorseLimit 2): the cool down differs on a horse
    add(1118, {{"HorseLimit", "1"}, {"TimePerCast", "30"}, {"TimePerCastOnHorse", "12"}});
    add(1119, {{"HorseLimit", "2"}, {"TimePerCast", "30"}, {"TimePerCastOnHorse", "12"}});
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
    c.gm_chat = true;   // "?gm ds <lua>" runs the script api for the hero (KillPlayer below)
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
    r.set_faction(-1);        // persist.NewRole: no faction yet (the proto's 0 would be Shaolin)
    r.set_faction_last(-1);
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

// The death of a player: KNpc 0x08089920 / DoDeath 0x080896C0 (the plain way, no PK), OnDeath 0x08088D50 (the
// experience and the money), the corpse 0x080833B0, KPlayer::Revive 0x080AD9F0 - docs/LINUX-SERVER.md §16.4
// Lua KillPlayer 0x08117BC0: KNpc::ReceiveDamage from oneself with twenty cells - seriesdamage_p 100,
// attackrating_v 50000, ignoredefense_p 1 and 200 000 000 .. 200 000 000 in the physics slot (type 0), no
// AR check, relation 0x1f: nothing blocks it, the character dies at once and the death of 16.4 follows
TEST_CASE("KillPlayer() of the script api: the unblockable self-hit kills, the corpse settles, the revive stands the character up", "[command][lua]")
{
    Arena a;
    a.h->player.exp = 0;   // nothing to lose: the point here is the hit
    a.h->cur.life = 5000;
    a.h->cur.defend = 100000;   // ignored: the AR check is off and the defense is ignored (cell 2)
    a.w.take_outbox();
    REQUIRE(a.w.chat(7, "?gm ds KillPlayer()"));
    CHECK(a.h->doing == KDoing::death);
    CHECK(a.h->cur.life == 0);
    bool death_told = false;
    for (const Packet& pk : a.w.take_outbox()) {
        if (pk.msg_id != static_cast<std::uint16_t>(jx::pb::G2C_ENTITY_ACTION)) continue;
        jx::pb::EntityAction act;
        REQUIRE(act.ParseFromString(pk.payload));
        if (act.entity_id() == a.hero.value && act.action() == jx::pb::ACTION_DEATH) death_told = true;
    }
    CHECK(death_told);
    // a second KillPlayer on the corpse changes nothing (ReceiveDamage refuses m_Doing 0xa / 0x15)
    REQUIRE(a.w.chat(7, "?gm ds KillPlayer()"));
    CHECK(a.h->doing == KDoing::death);
    a.ticks(static_cast<int>(a.h->frame_total) + 1);
    CHECK(a.h->doing == KDoing::revive);
    CHECK(a.w.revive_request(7, 1));
    CHECK(a.h->doing == KDoing::stand);
    CHECK(a.h->cur.life == a.h->life_max());
}

TEST_CASE("a player's death: the corpse waits, 2 percent of the level's experience and half the money go, the revive puts it back", "[command]")
{
    Arena a;
    KItemList* list = a.w.items_of(7);
    REQUIRE(list != nullptr);
    list->set_money(room_equipment, 100);
    a.h->player.exp = 1000;
    const std::int64_t level_exp = a.h->player.next_level_exp;   // KLevelAdd::GetLevelExp(5): 2 % (/ 50 below 100 000), at most 130 000, at most what is held
    const std::int64_t expected_loss = std::min<std::int64_t>(std::min<std::int64_t>(level_exp / 50, 130000), 1000);
    a.w.take_outbox();
    // a blow of 1000 kills: life 0, m_Doing 10 for the death frames, the loss of experience and money, a quarter on the ground
    int dealt = 0;
    CHECK(a.w.calc_damage(*a.h, *a.p, 1000, 1000, damage_physics, true, nullptr, &dealt, 0, false) == 0);
    CHECK(a.h->doing == KDoing::death);
    CHECK(a.h->cur.life == 0);
    CHECK(a.h->player.exp == 1000 - expected_loss);
    CHECK(list->money() == 50);
    bool death_told = false;
    for (const Packet& pk : a.w.take_outbox()) {
        if (pk.msg_id != static_cast<std::uint16_t>(jx::pb::G2C_ENTITY_ACTION)) continue;
        jx::pb::EntityAction act;
        REQUIRE(act.ParseFromString(pk.payload));
        if (act.entity_id() == a.hero.value && act.action() == jx::pb::ACTION_DEATH) death_told = true;
    }
    CHECK(death_told);
    // the revive request of a dead player before the corpse settled counts as dead too; until then a walk is refused
    CHECK_FALSE(a.w.move_request(7, Pos{2100, 2000}, 1));
    // the death frames run out: the corpse stays where it lies (m_Doing 21), the states are off, nothing counts on
    a.ticks(static_cast<int>(a.h->frame_total) + 1);
    CHECK(a.h->doing == KDoing::revive);
    CHECK(a.h->state_skills.empty());
    CHECK(a.h->cur.life == 0);
    a.ticks(5);
    CHECK(a.h->doing == KDoing::revive);
    // KPlayer::Revive(0): full life, mana and stamina, standing, fight mode off, at the revive point (the spawn point
    // of the map when none was set), ACTION_REVIVE to the clients around
    a.h->set_pos(Pos{2300, 2000});
    a.h->fight_mode = true;
    CHECK(a.w.revive_request(7, 2));
    CHECK(a.h->doing == KDoing::stand);
    CHECK(a.h->cur.life == a.h->life_max());
    CHECK(a.h->cur.mana == a.h->mana_max());
    CHECK_FALSE(a.h->fight_mode);
    CHECK(a.h->pos() == Pos{2000, 2000});
    bool revive_told = false;
    for (const Packet& pk : a.w.take_outbox()) {
        if (pk.msg_id != static_cast<std::uint16_t>(jx::pb::G2C_ENTITY_ACTION)) continue;
        jx::pb::EntityAction act;
        REQUIRE(act.ParseFromString(pk.payload));
        if (act.entity_id() == a.hero.value && act.action() == jx::pb::ACTION_REVIVE) revive_told = true;
    }
    CHECK(revive_told);
    // alive: a revive request is refused ("Client Want to Revive But he is no deaded!") - the character stands
    CHECK_FALSE(a.w.revive_request(7, 3));
    CHECK(a.h->doing == KDoing::stand);
    // a revive point set by SetTempRevPos (cells x 32, this map): the next death revives there
    a.h->player.revive_map = a.w.map_id();
    a.h->player.revive_x = 2500;
    a.h->player.revive_y = 2100;
    a.h->cur.life = 5;
    CHECK(a.w.calc_damage(*a.h, *a.p, 1000, 1000, damage_physics, true, nullptr, &dealt, 0, false) == 0);
    CHECK(a.h->doing == KDoing::death);
    CHECK(a.w.player_revive(*a.h, 2, false));   // type 2: where it lies, out of fight mode
    CHECK(a.h->pos() == Pos{2000, 2000});
    a.h->cur.life = 5;
    CHECK(a.w.calc_damage(*a.h, *a.p, 1000, 1000, damage_physics, true, nullptr, &dealt, 0, false) == 0);
    CHECK(a.w.revive_request(7, 4));
    CHECK(a.h->pos() == a.w.to_local(Pos{2500, 2100}));
}

// the create-npc skill 0x080E8770 (style 4): a npc of the state attribute createnpc {template, level, time} at the spot,
// named "<template> [<launcher>]", of the launcher's camp, gone when it dies (+0x1824) or when the player leaves
// (KPlayer::Clear 0x080B60A0); two records (+0x7d34) per stay, ChildSkillNum of each kind (Param1), G_SkillList_4 when
// none is free; nothing frees a record before the player leaves (nothing reads the time either)
TEST_CASE("style 4: the create-npc skill summons a npc at the spot with the launcher's camp; two records, freed on leave", "[command]")
{
    Arena a({1, 1115, 1116, 1117});
    a.h->camp = camp_justice;
    a.h->current_camp = camp_justice;
    a.w.take_outbox();
    REQUIRE(a.w.cast_skill_request(7, 1115, 2050, 2000, EntityId{}, 1));
    // the cast fires at the cast frame; the npc is made when that tick ends
    EntityId summon{};
    for (int i = 0; i < 40 && summon.value == 0; ++i) {
        a.ticks(1);
        const KPlayer& pl = a.w.mutable_entity(a.hero)->player;
        for (const KPlayer::KSummonRecord& r : pl.summons) {
            if (r.used && r.npc.value != 0) summon = r.npc;
        }
    }
    REQUIRE(summon.value != 0);
    a.h = a.w.mutable_entity(a.hero);   // the table may have moved
    a.p = a.w.mutable_entity(a.pig);
    KNpc* s = a.w.mutable_entity(summon);
    REQUIRE(s != nullptr);
    CHECK(s->name == "418 [Hero]");
    CHECK(s->level == 5);
    CHECK(s->series == 0);
    CHECK(s->kind == KNpcKind::monster);
    CHECK(s->camp == camp_justice);
    CHECK(s->current_camp == camp_justice);
    CHECK(s->remove_on_death);
    CHECK(s->summon_master == a.hero);
    CHECK(s->pos() == Pos{2050, 2000});
    CHECK(a.h->player.summon_free == 1);
    CHECK(a.h->player.summon_count(2) == 1);
    // a second one of that kind: ChildSkillNum 1 is reached (CanCastSkill 0x080E8F95, the cast itself 0x080E886E)
    const KSkill* sk = a.w.skills()->get(1115, 1);
    REQUIRE(sk != nullptr);
    int p1 = 2050, p2 = 2000;
    EntityId t{};
    CHECK_FALSE(a.w.can_cast_skill(*sk, *a.h, p1, p2, t));
    // another kind takes the last record
    REQUIRE(a.w.cast_skill_request(7, 1116, 2040, 2010, EntityId{}, 2));
    for (int i = 0; i < 40 && a.w.mutable_entity(a.hero)->player.summon_free != 0; ++i) a.ticks(1);
    a.h = a.w.mutable_entity(a.hero);
    CHECK(a.h->player.summon_free == 0);
    CHECK(a.h->player.summon_count(3) == 1);
    // no record left: G_SkillList_4 to the player, no cast (0x080E8FEC / 0x080E8A94)
    a.w.take_outbox();
    const KSkill* sk3 = a.w.skills()->get(1117, 1);
    REQUIRE(sk3 != nullptr);
    CHECK_FALSE(a.w.can_cast_skill(*sk3, *a.h, p1, p2, t));
    bool told = false;
    for (const Packet& pk : a.w.take_outbox()) {
        if (pk.msg_id != static_cast<std::uint16_t>(jx::pb::G2C_CHAT_MSG)) continue;
        jx::pb::ChatMsg m;
        REQUIRE(m.ParseFromString(pk.payload));
        if (m.text().size() > 20 && m.text()[0] == 'S') told = true;   // "So luong linh danh thue ..." (UTF-8)
    }
    CHECK(told);
    // the summon dies: out of the world when its death frames end, and the record stays used (nothing frees it)
    s = a.w.mutable_entity(summon);
    s->cur.life = 1;
    int dealt = 0;
    CHECK(a.w.calc_damage(*s, *a.w.mutable_entity(a.pig), 100, 100, damage_physics, true, nullptr, &dealt, 0, false) == 0);
    for (int i = 0; i < 80 && a.w.mutable_entity(summon) != nullptr; ++i) a.ticks(1);
    CHECK(a.w.mutable_entity(summon) == nullptr);
    a.h = a.w.mutable_entity(a.hero);
    CHECK(a.h->player.summon_free == 0);
    CHECK(a.h->player.summon_count(2) == 1);
    // the player leaves: KPlayer::Clear takes the other summon out and frees the records
    EntityId other{};
    for (const KPlayer::KSummonRecord& r : a.h->player.summons) {
        if (r.used && r.npc != summon && r.npc.value != 0) other = r.npc;
    }
    REQUIRE(other.value != 0);
    REQUIRE(a.w.mutable_entity(other) != nullptr);
    REQUIRE(a.w.remove_player(7));
    CHECK(a.w.mutable_entity(other) == nullptr);
}

// KNpc::SetHorse 0x0807D520 from the fight side: mounting while hidden breaks the hiding (0x0807D4C0), HorseLimit 1 / 2
// of CanCastSkill (0x080E8ED2 / 0x080E8CBB), SetSkillCoolTime 0x0808482E takes the OnHorse column while riding
TEST_CASE("a horse and the skills: mounting breaks the hiding, HorseLimit 1 / 2, the cool down of the horse column", "[command]")
{
    Arena a({1, 1101, 1108, 1118, 1119});
    // [hide] 1 on oneself (skill 1108, see the hide test), then the mount
    REQUIRE(a.w.cast_skill_request(7, 1108, -1, 0, a.hero, 1));
    for (int i = 0; i < 40 && a.h->hide == 0; ++i) a.ticks(1);
    REQUIRE(a.h->hide > 0);
    a.w.set_horse(*a.h, 1);
    CHECK(a.h->horse == 1);
    CHECK(a.h->hide == 0);
    // HorseLimit: 1118 only on foot, 1119 only on a horse
    const KSkill* foot = a.w.skills()->get(1118, 1);
    const KSkill* rider = a.w.skills()->get(1119, 1);
    REQUIRE((foot != nullptr && rider != nullptr));
    int p1 = -1, p2 = 0;
    EntityId t = a.pig;
    CHECK_FALSE(a.w.can_cast_skill(*foot, *a.h, p1, p2, t));
    p1 = -1; t = a.pig;
    CHECK(a.w.can_cast_skill(*rider, *a.h, p1, p2, t));
    // the cool down on a horse: TimePerCastOnHorse 12 instead of 30
    a.w.set_skill_cool_time(*a.h, 1119, 1);
    CHECK(a.h->skill_list.next_cast_time(1119) == a.w.tick_count() + 12);
    a.w.set_horse(*a.h, 0);
    CHECK(a.h->horse == 0);
    p1 = -1; t = a.pig;
    CHECK(a.w.can_cast_skill(*foot, *a.h, p1, p2, t));
    p1 = -1; t = a.pig;
    CHECK_FALSE(a.w.can_cast_skill(*rider, *a.h, p1, p2, t));
    a.w.set_skill_cool_time(*a.h, 1118, 1);
    CHECK(a.h->skill_list.next_cast_time(1118) == a.w.tick_count() + 30);
    // frozen_action: SetHorse does nothing
    a.h->cur.frozen_action = true;
    a.w.set_horse(*a.h, 1);
    CHECK(a.h->horse == 0);
}

namespace {

// the three factions the faction tests use: Shaolin (metal, C_JUSTICE), Tang Men (wood, C_BALANCE), Wudu (wood, C_EVIL)
std::shared_ptr<const jx::zone::KFaction> faction_table()
{
    auto t = std::make_shared<jx::zone::KFaction>();
    t->set(0, 0, 1, "shaolin", "Thieu Lam phai");
    t->set(2, 1, 3, "tangmen", "Duong Mon");
    t->set(3, 1, 2, "wudu", "Ngu Doc Giao");
    t->set_names("Moi nhap giang ho ", "giang ho hiep khach");
    t->set_skills(0, {14, 8, 10});
    return t;
}

template <typename Msg>
std::vector<Msg> faction_packets(std::vector<Packet> all, jx::pb::MsgId id)
{
    std::vector<Msg> out;
    for (const Packet& pk : all) {
        if (pk.msg_id != static_cast<std::uint16_t>(id)) continue;
        Msg m;
        REQUIRE(m.ParseFromString(pk.payload));
        out.push_back(m);
    }
    return out;
}

} // namespace

TEST_CASE("KFaction: the table of 0x08060C70 and the record of KPlayer+0x59cc", "[faction]")
{
    Quiet q;
    const auto t = faction_table();
    // 0x08060C00: the exact code name; a series above 4 (unsigned) or an empty name refuses
    CHECK(t->id_by_name(0, "shaolin") == 0);
    CHECK(t->id_by_name(4, "wudu") == 3);
    CHECK(t->id_by_name(0, "Shaolin") == -1);
    CHECK(t->id_by_name(0, "") == -1);
    CHECK(t->id_by_name(5, "shaolin") == -1);
    CHECK(t->id_by_name(-1, "shaolin") == -1);
    // 0x08060BB0: the entry with that index AND that series
    CHECK(t->allows(0, 0));
    CHECK_FALSE(t->allows(1, 0));
    CHECK(t->allows(1, 2));
    CHECK_FALSE(t->allows(0, 11));
    // an entry the file never named keeps the ctor's index / series 0 / camp 1
    REQUIRE(t->entry(1) != nullptr);
    CHECK(t->entry(1)->camp == 1);
    CHECK(t->entry(1)->name.empty());
    CHECK(t->entry(11) == nullptr);
    REQUIRE(t->skills(0) != nullptr);
    CHECK(t->skills(0)->size() == 3);
    CHECK(t->skills(2) == nullptr);

    jx::zone::KPlayerFaction r;
    CHECK((r.current == -1 && r.first == -1 && r.last == -1 && r.count == 0));
    // 0x080C2610 / 0x080C2680: never joined -> C_BEGIN and ""
    CHECK(r.camp(t.get()) == 0);
    CHECK(r.name(t.get()).empty());
    CHECK(r.last_name(t.get()).empty());
    // 0x080C26F0: a wood character cannot join Shaolin; a metal one can
    CHECK_FALSE(r.add(*t, 1, 0));
    CHECK(r.count == 0);
    CHECK(r.add(*t, 0, 0));
    CHECK((r.current == 0 && r.first == 0 && r.last == 0 && r.count == 1));
    CHECK(r.camp(t.get()) == 1);
    CHECK(r.name(t.get()) == "shaolin");
    // 0x080C25F0: leaving keeps the count and the last; the name becomes G_FACTION_OLD, the camp C_FREE
    r.clear_current();
    CHECK((r.current == -1 && r.last == 0 && r.count == 1));
    CHECK(r.camp(t.get()) == 4);
    CHECK(r.name(t.get()) == "giang ho hiep khach");
    CHECK(r.last_name(t.get()) == "shaolin");
    // a second faction: the first stays, the last moves, the count grows (a wood character this time)
    jx::zone::KPlayerFaction w;
    CHECK(w.add(*t, 1, 2));
    CHECK(w.add(*t, 1, 3));
    CHECK((w.current == 3 && w.first == 2 && w.last == 3 && w.count == 2));
    CHECK(w.camp(t.get()) == 2);
    CHECK(w.last_name(t.get()) == "wudu");
    // 0x080C25C0
    w.reset();
    CHECK((w.current == -1 && w.first == -1 && w.last == -1 && w.count == 0));
    // without a table nothing has a name or a faction camp
    CHECK(r.name(nullptr).empty());
    CHECK(r.camp(nullptr) == 4);

    // faction.json of jxassets export-faction
    const std::filesystem::path dir = std::filesystem::temp_directory_path() / "jx_faction_test";
    std::filesystem::create_directories(dir);
    const std::string file = (dir / "faction.json").string();
    {
        std::ofstream out(file, std::ios::binary);
        out << R"({"source":"x","factions":[{"index":0,"series":0,"camp":1,"name":"shaolin","show_name":"Thieu Lam phai"},)"
               R"({"index":2,"series":1,"camp":3,"name":"tangmen","show_name":"Duong Mon"}],"new_name":"Moi nhap giang ho ",)"
               R"("old_name":"giang ho hiep khach","skills":{"0":[14,8],"2":[45]}})";
    }
    std::string error;
    auto loaded = jx::zone::KFaction::load(file, &error);
    REQUIRE(loaded.has_value());
    CHECK(loaded->id_by_name(0, "shaolin") == 0);
    CHECK(loaded->id_by_name(0, "tangmen") == 2);
    CHECK(loaded->entry(2)->camp == 3);
    CHECK(loaded->entry(2)->show_name == "Duong Mon");
    CHECK(loaded->old_name() == "giang ho hiep khach");
    REQUIRE(loaded->skills(2) != nullptr);
    CHECK(loaded->skills(2)->at(0) == 45);
    CHECK_FALSE(jx::zone::KFaction::load((dir / "missing.json").string(), &error).has_value());
    std::filesystem::remove_all(dir);
}

TEST_CASE("SetFaction / ClearFaction / the camps / AddMagic through the script api", "[faction]")
{
    Quiet q;
    KSubWorldConfig cfg = small_world();
    cfg.faction = faction_table();
    KSubWorld w(cfg);
    EntityId hero;
    Pos at;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}, {1, 1101}), hero, at) == jx::pb::RESULT_OK);
    KNpc* h = w.mutable_entity(hero);
    REQUIRE(h != nullptr);
    CHECK(h->player.faction.current == -1);
    w.take_outbox();
    // KPlayer::SetFaction 0x080AEEC0: the record, the camp of the faction (0x0807B7B0 -> the 0x59 packet), the 0x7b packet
    REQUIRE(w.chat(7, "?gm ds SetFaction(\"shaolin\")"));
    CHECK(h->player.faction.current == 0);
    CHECK(h->player.faction.count == 1);
    CHECK(h->camp == 1);
    auto out = w.take_outbox();
    auto camps = faction_packets<jx::pb::EntityCamp>(out, jx::pb::G2C_ENTITY_CAMP);
    REQUIRE(camps.size() == 1);
    CHECK(camps[0].entity_id() == hero.value);
    CHECK(camps[0].camp() == 1);
    auto fs = faction_packets<jx::pb::PlayerFaction>(out, jx::pb::G2C_PLAYER_FACTION);
    REQUIRE(fs.size() == 1);
    CHECK((fs[0].camp() == 1 && fs[0].faction() == 0 && fs[0].faction_last() == 0 && fs[0].faction_count() == 1));
    // the login sync carries the record too (0x080A9750 +0xb0e / +0xb12)
    w.send_player_attrib(7);
    auto attribs = faction_packets<jx::pb::PlayerAttribSync>(w.take_outbox(), jx::pb::G2C_PLAYER_ATTRIB);
    REQUIRE(attribs.size() == 1);
    CHECK((attribs[0].faction() == 0 && attribs[0].faction_last() == 0));
    // a wood faction refuses a metal character (0x08060BB0): the record does not move, nothing is sent
    REQUIRE(w.chat(7, "?gm ds SetFaction(\"tangmen\")"));
    CHECK(h->player.faction.current == 0);
    CHECK(h->player.faction.count == 1);
    CHECK(faction_packets<jx::pb::PlayerFaction>(w.take_outbox(), jx::pb::G2C_PLAYER_FACTION).empty());
    // GetFaction / GetFactionNumber / GetLastFactionNumber / GetLastAddFaction / GetCamp, read back through SetCamp
    REQUIRE(w.chat(7, "?gm ds if GetFaction() == \"shaolin\" and GetFactionNumber() == 0 and GetLastFactionNumber() == 0 and GetLastAddFaction() == \"shaolin\" and GetCamp() == 1 then SetCurCamp(5) end"));
    CHECK(h->current_camp == 5);
    CHECK(h->camp == 1);
    auto cur = faction_packets<jx::pb::EntityCamp>(w.take_outbox(), jx::pb::G2C_ENTITY_CAMP);
    REQUIRE(cur.size() == 1);
    CHECK((cur[0].camp() == 1 && cur[0].current_camp() == 5));
    REQUIRE(w.chat(7, "?gm ds if GetCurCamp() == 5 then SetCamp(2) end"));
    CHECK(h->camp == 2);
    // a negative camp is ignored (0x0811B31D)
    REQUIRE(w.chat(7, "?gm ds SetCamp(-1)"));
    CHECK(h->camp == 2);
    // the record survives a save and a load (LoadFrom 0x080C1A62: current, last, count; the first is not kept)
    jx::pb::RoleData saved;
    h->player.save_to(*h, saved);
    CHECK((saved.faction() == 0 && saved.faction_last() == 0 && saved.faction_count() == 1));
    // KPlayer::ClearFaction 0x080AEDE0: SetFaction("") - the current -1, the camp C_FREE, the 0x7c packet; the name
    // of a character that left is G_FACTION_OLD
    REQUIRE(w.chat(7, "?gm ds SetFaction(\"\")"));
    CHECK(h->player.faction.current == -1);
    CHECK(h->player.faction.last == 0);
    CHECK(h->player.faction.count == 1);
    CHECK(h->camp == 4);
    fs = faction_packets<jx::pb::PlayerFaction>(w.take_outbox(), jx::pb::G2C_PLAYER_FACTION);
    REQUIRE(fs.size() == 1);
    CHECK((fs[0].camp() == 4 && fs[0].faction() == -1 && fs[0].faction_last() == 0 && fs[0].faction_count() == 1));
    REQUIRE(w.chat(7, "?gm ds if GetFaction() == \"giang ho hiep khach\" and GetFactionNumber() == -1 then SetCamp(6) end"));
    CHECK(h->camp == 6);
    // SetLastFactionNumber / ClearFactionRecord
    REQUIRE(w.chat(7, "?gm ds SetLastFactionNumber(3)"));
    CHECK(h->player.faction.last == 3);
    REQUIRE(w.chat(7, "?gm ds ClearFactionRecord()"));
    CHECK((h->player.faction.current == -1 && h->player.faction.last == -1 && h->player.faction.count == 0));
    REQUIRE(w.chat(7, "?gm ds if GetFaction() == \"\" then SetCamp(0) end"));
    CHECK(h->camp == 0);
    // joining again counts again
    REQUIRE(w.chat(7, "?gm ds SetFaction(\"shaolin\")"));
    CHECK((h->player.faction.current == 0 && h->player.faction.first == 0 && h->player.faction.count == 1));
    w.take_outbox();
    // AddMagic 0x0812C430: the skill at level 0 (a level above 1 needs an instance; 1102 has none at 70)
    CHECK(h->skill_list.find_same(1102) == 0);
    REQUIRE(w.chat(7, "?gm ds AddMagic(1102)"));
    CHECK(h->skill_list.find_same(1102) != 0);
    CHECK(h->skill_list.get_level(1102) == 0);
    auto levels = faction_packets<jx::pb::SkillLevelSync>(w.take_outbox(), jx::pb::G2C_SKILL_LEVEL);
    REQUIRE(levels.size() == 1);
    CHECK(levels[0].skill_id() == 1102);
    REQUIRE(w.chat(7, "?gm ds AddMagic(1103, 70)"));
    CHECK(h->skill_list.find_same(1103) == 0);
    REQUIRE(w.chat(7, "?gm ds AddMagic(1103, 1)"));
    CHECK(h->skill_list.get_level(1103) == 1);
    REQUIRE(w.chat(7, "?gm ds AddMagic(0)"));
    CHECK(faction_packets<jx::pb::SkillLevelSync>(w.take_outbox(), jx::pb::G2C_SKILL_LEVEL).size() == 1);
}

TEST_CASE("SetFaction without a table: refused, logged", "[faction]")
{
    Quiet q;
    KSubWorldConfig cfg = small_world();
    KSubWorld w(cfg);
    EntityId hero;
    Pos at;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}, {1}), hero, at) == jx::pb::RESULT_OK);
    KNpc* h = w.mutable_entity(hero);
    w.take_outbox();
    REQUIRE(w.chat(7, "?gm ds SetFaction(\"shaolin\")"));
    CHECK(h->player.faction.current == -1);
    CHECK(faction_packets<jx::pb::PlayerFaction>(w.take_outbox(), jx::pb::G2C_PLAYER_FACTION).empty());
}

TEST_CASE("the 0x87 packet: a skill's state on a player goes to its client, its removal too", "[state]")
{
    Quiet q;
    KSubWorldConfig cfg = small_world();
    KSubWorld w(cfg);
    EntityId hero;
    Pos at;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}, {1, 1101}), hero, at) == jx::pb::RESULT_OK);
    KNpc* h = w.mutable_entity(hero);
    REQUIRE(h != nullptr);
    w.take_outbox();
    // a state of skill 1101 level 1 for 36 frames with two attributes (SetStateSkillEffect 0x08086260, a new node)
    std::array<KMagicAttrib, 2> states{};
    states[0].type = 1;
    states[0].value = {5, 0, 0};
    states[1].type = 2;
    states[1].value = {-3, 1, 0};
    REQUIRE(w.set_state_skill_effect(*h, hero, 1101, 1, states.data(), 2, 36, 0, false, 0, false, 0) == 0);
    auto out = faction_packets<jx::pb::EntityState>(w.take_outbox(), jx::pb::G2C_ENTITY_STATE);
    REQUIRE(out.size() == 1);
    CHECK(out[0].entity_id() == hero.value);
    CHECK(out[0].skill_id() == 1101);
    CHECK(out[0].level() == 1);
    CHECK(out[0].time() == 36);
    CHECK_FALSE(out[0].removed());
    REQUIRE(out[0].states_size() == 2);
    CHECK((out[0].states(0).type() == 1 && out[0].states(0).v0() == 5));
    CHECK((out[0].states(1).type() == 2 && out[0].states(1).v0() == -3 && out[0].states(1).v1() == 1));
    // the same skill again while the node lives: no new packet (0x08086730 returns before 0x08086892)
    REQUIRE(w.set_state_skill_effect(*h, hero, 1101, 1, states.data(), 2, 36, 0, false, 0, false, 0) == 36);
    CHECK(faction_packets<jx::pb::EntityState>(w.take_outbox(), jx::pb::G2C_ENTITY_STATE).empty());
    // RemoveStateSkillEffect with notify: the empty packet (level 63 / time 0 of the binary = removed here)
    w.remove_state_skill_effect(*h, 1101, true);
    out = faction_packets<jx::pb::EntityState>(w.take_outbox(), jx::pb::G2C_ENTITY_STATE);
    REQUIRE(out.size() == 1);
    CHECK(out[0].removed());
    CHECK(out[0].skill_id() == 1101);
    CHECK(out[0].states_size() == 0);
    // without notify (0x0807D310(npc, id, 0) of a cast) nothing is sent
    REQUIRE(w.set_state_skill_effect(*h, hero, 1101, 1, states.data(), 2, 36, 0, false, 0, false, 0) == 0);
    w.take_outbox();
    w.remove_state_skill_effect(*h, 1101, false);
    CHECK(faction_packets<jx::pb::EntityState>(w.take_outbox(), jx::pb::G2C_ENTITY_STATE).empty());
    // a monster's state is nobody's packet (0x08086902: only a player's npc)
    EntityId pig = w.spawn_npc("pig", Pos{2050, 2000}, 418, 0, KNpcKind::monster);
    KNpc* p = w.mutable_entity(pig);
    REQUIRE(p != nullptr);
    w.take_outbox();
    REQUIRE(w.set_state_skill_effect(*p, hero, 1101, 1, states.data(), 2, 36, 0, false, 0, false, 0) == 0);
    CHECK(faction_packets<jx::pb::EntityState>(w.take_outbox(), jx::pb::G2C_ENTITY_STATE).empty());
}

TEST_CASE("the skill tip: the zone answers cost, range and the level's attributes for the level shown and the next", "[command]")
{
    // KSkill::GetDesc 0x006FBC90 of the 2.0 client asks the level script for the numbers; here the zone's own KSkill
    Arena a;
    a.w.take_outbox();
    a.w.skill_desc_request(7, 1101, 1);
    auto descs = faction_packets<jx::pb::SkillDesc>(a.w.take_outbox(), jx::pb::G2C_SKILL_DESC);
    REQUIRE(descs.size() == 1);
    const auto& d = descs[0];
    CHECK(d.skill_id() == 1101);
    CHECK(d.max_level() == 20);
    REQUIRE(d.with_cur());
    CHECK(d.cur().level() == 1);
    CHECK(d.cur().cost() == 10);             // the fixture: 10 mana
    CHECK(d.cur().attack_radius() == 100);
    bool life = false;
    for (const auto& at : d.cur().attribs()) {
        if (at.name() == "life_v") life = true;
    }
    CHECK(life);                             // LvlSetting1 life_v of the level script
    REQUIRE(d.with_next());
    CHECK(d.next().level() == 2);
    // the zone answers for the level it holds (0x006233B0 of the 2.0 client), whatever the client asked: 1101 at 1 again
    a.w.skill_desc_request(7, 1101, 0);
    descs = faction_packets<jx::pb::SkillDesc>(a.w.take_outbox(), jx::pb::G2C_SKILL_DESC);
    REQUIRE(descs.size() == 1);
    CHECK((descs[0].with_cur() && descs[0].held_level() == 1 && descs[0].cur().level() == 1));
    // not held (1108 is in the table, not in the list): the level asked, 0 -> the next level only; at the top level no next;
    // an unknown skill: an empty answer
    a.w.skill_desc_request(7, 1108, 0);
    descs = faction_packets<jx::pb::SkillDesc>(a.w.take_outbox(), jx::pb::G2C_SKILL_DESC);
    REQUIRE(descs.size() == 1);
    CHECK((!descs[0].with_cur() && descs[0].with_next() && descs[0].next().level() == 1));
    a.w.skill_desc_request(7, 1108, 20);
    descs = faction_packets<jx::pb::SkillDesc>(a.w.take_outbox(), jx::pb::G2C_SKILL_DESC);
    REQUIRE(descs.size() == 1);
    CHECK((descs[0].with_cur() && !descs[0].with_next()));
    a.w.skill_desc_request(7, 1999, 1);
    descs = faction_packets<jx::pb::SkillDesc>(a.w.take_outbox(), jx::pb::G2C_SKILL_DESC);
    REQUIRE(descs.size() == 1);
    CHECK((!descs[0].with_cur() && !descs[0].with_next() && descs[0].max_level() == 0));
}

TEST_CASE("an aura: SetAura 0x08087290 keeps an IsAura skill held, ProcessState casts its child every ten frames (0x080873B0)", "[command][aura]")
{
    Arena a({1, 1101, 1102, 1103});
    a.w.take_outbox();
    // not an aura, not held, out of range: nothing is kept (and the icons are marked 2)
    a.w.set_aura(*a.h, 1101);
    CHECK(a.h->aura_skill_id == 0);
    a.w.set_aura(*a.h, 1108);
    CHECK(a.h->aura_skill_id == 0);
    a.w.set_aura(*a.h, 2000);
    CHECK(a.h->aura_skill_id == 0);
    // the aura 1103 held at level 1: kept; the refusals above marked the icons 2, so the next frame rebuilds them
    // (0x08087160) with the aura's StateSpecialId 45 and tells the watchers (the 0x7a packet)
    a.w.set_aura_request(7, 1103);
    CHECK(a.h->aura_skill_id == 1103);
    a.h->watchers = {7};
    a.w.take_outbox();
    a.ticks(1);
    auto icons = faction_packets<jx::pb::EntityStateIcons>(a.w.take_outbox(), jx::pb::G2C_STATE_ICONS);
    REQUIRE(!icons.empty());
    bool told = false;
    for (const auto v : icons.back().icons()) {
        if (v == 45) told = true;
    }
    CHECK(told);
    CHECK(a.h->state_flag == 0);
    bool icon = false;
    for (const auto& ic : a.h->state_icons) {
        if (ic.id == 45) icon = true;
    }
    CHECK(icon);
    // every GAME_UPDATE_TIME frames the child 1102 is cast at the hero's spot: the buff state 1102 (armordefense_v, 60
    // frames) lands on the hero
    CHECK(a.h->state_of(1102) == nullptr);
    a.ticks(11);
    CHECK(a.h->state_of(1102) != nullptr);
    // a hidden npc casts no aura effect (0x080873C9)
    // ForbitAura: the request clears instead of setting
    a.h->player.forbid_aura = true;
    a.w.set_aura_request(7, 1103);
    CHECK(a.h->aura_skill_id == 0);
    a.h->player.forbid_aura = false;
    a.w.set_aura_request(7, 1103);
    CHECK(a.h->aura_skill_id == 1103);
    // the client sets a non-aura right skill: SetAura(0) clears (0x005EA8B4)
    a.w.set_aura_request(7, 0);
    CHECK(a.h->aura_skill_id == 0);
}

TEST_CASE("the skill tip names the skills of a level the way 0x006FAA00 / 0x006F7F70 of the 2.0 client do", "[command]")
{
    Arena a({1, 1101, 1102, 1120, 1121});
    a.w.take_outbox();
    // the asker holds 1120 at level 1: its tip names 1101 (append skill, flags 1) at min(held 1, listed 3) = 1 with 1101's
    // own life_v line, then 1102 (ShowEvent bit 2 -> FlySkillId, flags 0) at 1120's own level with its armordefense_v
    a.w.skill_desc_request(7, 1120, 1);
    auto descs = faction_packets<jx::pb::SkillDesc>(a.w.take_outbox(), jx::pb::G2C_SKILL_DESC);
    REQUIRE(descs.size() == 1);
    const auto& d = descs[0];
    REQUIRE(d.with_cur());
    CHECK(d.held_level() == 1);
    CHECK(d.level_inc() == 0);
    REQUIRE(d.cur().related_size() == 2);
    CHECK(d.cur().related(0).skill_id() == 1101);
    CHECK(d.cur().related(0).level() == 1);
    CHECK(d.cur().related(0).flags() == 1);
    REQUIRE(d.cur().related(0).attribs_size() >= 1);
    CHECK(d.cur().related(0).attribs(0).name() == "life_v");
    CHECK(d.cur().related(1).skill_id() == 1102);
    CHECK(d.cur().related(1).level() == 1);
    CHECK(d.cur().related(1).flags() == 0);
    REQUIRE(d.cur().related(1).attribs_size() >= 1);
    CHECK(d.cur().related(1).attribs(0).name() == "armordefense_v");
    // the next level (2) names 1102 at level 2 (EventSkillLevel -1 = the level itself)
    REQUIRE(d.with_next());
    REQUIRE(d.next().related_size() == 2);
    CHECK(d.next().related(1).level() == 2);
    // 1101's own tip: the enhance map holds 1120's addskilldamage1 (25) - G_Skills_39 - and no named skill
    a.w.skill_desc_request(7, 1101, 1);
    descs = faction_packets<jx::pb::SkillDesc>(a.w.take_outbox(), jx::pb::G2C_SKILL_DESC);
    REQUIRE(descs.size() == 1);
    CHECK(descs[0].enhance() == 25);
    CHECK(descs[0].cur().related_size() == 0);
    // 1121 names 1108, which the asker does not hold: level 0 -> nothing (0x006F7FC4)
    a.w.skill_desc_request(7, 1121, 1);
    descs = faction_packets<jx::pb::SkillDesc>(a.w.take_outbox(), jx::pb::G2C_SKILL_DESC);
    REQUIRE(descs.size() == 1);
    CHECK(descs[0].cur().related_size() == 0);
    // an increment on 1101 (an add_level_inc node): the level shown is the current one and the tip says so
    auto host = a.w.skill_host(*a.h);
    a.h->skill_list.add_level_inc(1101, 2, host);
    a.w.skill_desc_request(7, 1101, 1);
    descs = faction_packets<jx::pb::SkillDesc>(a.w.take_outbox(), jx::pb::G2C_SKILL_DESC);
    REQUIRE(descs.size() == 1);
    CHECK(descs[0].held_level() == 3);
    CHECK(descs[0].level_inc() == 2);
    CHECK((descs[0].with_cur() && descs[0].cur().level() == 3));
    CHECK(descs[0].equip_percent() == 0);
    CHECK_FALSE(descs[0].with_modifier());
    // G_Skills_76 (0x006FC348 / 0x005EC4F0): the asker's magicdamage_p and a state modifier aimed at the skill go out with the tip
    a.h->cur.magic_damage_percent = 12;
    a.h->state_modifier = KStateModifier{1101, magic_armordefense_v, 2, 7};
    a.w.skill_desc_request(7, 1101, 1);
    descs = faction_packets<jx::pb::SkillDesc>(a.w.take_outbox(), jx::pb::G2C_SKILL_DESC);
    REQUIRE(descs.size() == 1);
    CHECK(descs[0].equip_percent() == 12);
    REQUIRE(descs[0].with_modifier());
    CHECK(descs[0].modifier().name() == "armordefense_v");
    CHECK(descs[0].modifier().v2() == 7);
    CHECK(descs[0].modifier().v0() == 0);
    a.w.skill_desc_request(7, 1102, 1);   // another skill: no modifier line
    descs = faction_packets<jx::pb::SkillDesc>(a.w.take_outbox(), jx::pb::G2C_SKILL_DESC);
    REQUIRE(descs.size() == 1);
    CHECK_FALSE(descs[0].with_modifier());
    CHECK(descs[0].equip_percent() == 12);
}


TEST_CASE("a template's aura and passive skill: InitNpcLevelData 0x080A37A0 fills cells 5 / 6 at creation (0x08085250), a placed npc casts cell 5 every ten frames (0x0808BAF6)", "[command][aura][template]")
{
    // 950: a monk with the aura 1103 (AuraSkillLevel "0|1" = the npc's level) and the passive 1130 ("1|0" = 1);
    // 951: its twin whose aura level "0|30" clamps to 64 (0x080A3AB2) - held in cell 5 but never cast (0x080873B0 refuses > 63)
    KSubWorldConfig c = small_world();
    KNpcTemplateSet set;
    KNpcTemplate t;
    t.id = 950;
    t.name = "monk";
    t.kind = kind_normal;
    t.camp = camp_animal;
    t.life_param = 50;
    t.cells["AuraSkillId"] = "1103";
    t.cells["AuraSkillLevel"] = "0|1";
    t.cells["PasstSkillId"] = "1130";
    t.cells["PasstSkillLevel"] = "1|0";
    set.add(t);
    t.id = 951;
    t.cells["AuraSkillLevel"] = "0|30";
    set.add(t);
    c.templates = std::make_shared<const KNpcTemplateSet>(std::move(set));
    Quiet quiet;
    KSubWorld w(c);
    const KNpcLevelData d = KNpcTemplateSet::level_data(*c.templates->find(950), 3, 0, c.scripts.get());
    CHECK(d.aura_skill_id == 1103);
    CHECK(d.aura_skill_level == 3);
    CHECK(d.passive_skill_id == 1130);
    CHECK(d.passive_skill_level == 1);
    const KSkill* passive = w.skill_instance(1130, 1);
    REQUIRE(passive != nullptr);
    CHECK(passive->row.style == skill_style_passivity_npc_state);
    CHECK(passive->state_attrib_count > 0);
    Pos at;
    EntityId hero;
    REQUIRE(w.spawn_player(7, role(70, "Hero", Pos{2000, 2000}, {1}), hero, at) == jx::pb::RESULT_OK);
    // a placement (+0x181c = 1): the region loader's Add -> 0x08085250
    const EntityId monk = w.spawn_npc("monk", Pos{2100, 2000}, 950, 0, KNpcKind::monster, 3, 0, 1);
    KNpc* m = w.mutable_entity(monk);
    REQUIRE(m != nullptr);
    CHECK(m->boss_flag == 1);
    const KNpcSkill* c5 = m->skill_list.cell(5);
    REQUIRE(c5 != nullptr);
    CHECK(c5->id == 1103);
    CHECK(c5->current_level == 3);
    const KNpcSkill* c6 = m->skill_list.cell(6);
    REQUIRE(c6 != nullptr);
    CHECK(c6->id == 1130);
    CHECK(c6->current_level == 1);
    CHECK(m->state_of(1130) != nullptr);   // KSkill::Cast(passive, npc, -1, npc): its states on itself for good
    CHECK(m->state_of(1102) == nullptr);
    for (int i = 0; i < 11; ++i) w.tick();
    m = w.mutable_entity(monk);
    REQUIRE(m != nullptr);
    CHECK(m->state_of(1102) != nullptr);   // the aura's child every tenth frame (0x0808BAF6 -> 0x080873B0)
    // a skill-made npc (KNpcSet::Add with bKind 0): no cells 5 / 6, no aura
    const EntityId plain = w.spawn_npc("plain", Pos{2200, 2000}, 950, 0, KNpcKind::monster, 3, 0, 0);
    KNpc* p = w.mutable_entity(plain);
    REQUIRE(p != nullptr);
    CHECK(p->boss_flag == 0);
    CHECK(p->skill_list.cell(5)->id == 0);
    CHECK(p->state_of(1130) == nullptr);
    // the clamped twin: cell 5 at 64, nothing cast
    const EntityId twin = w.spawn_npc("twin", Pos{2300, 2000}, 951, 0, KNpcKind::monster, 3, 0, 1);
    KNpc* tw = w.mutable_entity(twin);
    REQUIRE(tw != nullptr);
    CHECK(tw->skill_list.cell(5)->id == 1103);
    CHECK(tw->skill_list.cell(5)->current_level == 64);
    for (int i = 0; i < 11; ++i) w.tick();
    tw = w.mutable_entity(twin);
    REQUIRE(tw != nullptr);
    CHECK(tw->state_of(1102) == nullptr);
    p = w.mutable_entity(plain);
    REQUIRE(p != nullptr);
    CHECK(p->state_of(1102) == nullptr);
}
