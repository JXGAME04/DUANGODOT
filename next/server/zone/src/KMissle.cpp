// KMissle.cpp - the missiles of the old core (Core/Src/KMissle.cpp, the CastMissles part of
// KSkills.cpp) the way the JX2 server runs them (jx_linux_y; every address in
// docs/LINUX-SERVER.md §13).  All of it is KSubWorld's: a missile needs the map, the npcs, the
// random numbers and the blows of KNpc.cpp.
//
// A word on the coordinates.  The binary keeps a missile as region + cell + offset (1/1024 of a
// unit inside the cell); the zone keeps the absolute cell (32 units) and the same offset, which is
// what every rule below reads, so the region bookkeeping (the neighbour lookups of 0x08074F10, the
// deferred move nodes 0xfa2) has no counterpart: a step off the map is the missing neighbour
// region of the binary.  What the binary computes with those - the cell distances of the
// collisions, the edge case of an offset of exactly 32 x 1024 - comes out the same.
#include "jx/zone/KSubWorld.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <utility>

#include <nlohmann/json.hpp>

#include "jx/log.hpp"
#include "jx/zone/KMagicAttribId.h"
#include "jx/zone/KMath.h"
#include "jx/zone/KNpcAI.h"

namespace jx::zone {

namespace {

// KTabFile::GetInteger for a cell: the default for an empty cell, strtol otherwise
int cell_int(const std::unordered_map<std::string, std::string>& cells, const char* column, int def = 0)
{
    const auto it = cells.find(column);
    if (it == cells.end() || it->second.empty()) return def;
    return static_cast<int>(std::strtol(it->second.c_str(), nullptr, 10));
}

// a npc's spot in 1/1024 units (the binary's cell + offset; the zone simulates in 1/256)
std::int64_t npc_x1024(const KNpc& e) noexcept { return e.fx * (1024 / kSub); }
std::int64_t npc_y1024(const KNpc& e) noexcept { return e.fy * (1024 / kSub); }

int npc_cell_x(const KNpc& e) noexcept { return e.pos().x / kMissleCell; }
int npc_cell_y(const KNpc& e) noexcept { return e.pos().y / kMissleCell; }

// the division toward zero the compiler emits for x / 2 and x / 4 (shr 31 / add / sar)
int half_toward_zero(int v) noexcept { return v / 2; }
int quarter_toward_zero(int v) noexcept { return v / 4; }

// (int)sqrt of an integer distance, truncated like the fistp of the binary (control word 0xC)
int int_sqrt(std::int64_t v) noexcept { return static_cast<int>(std::sqrt(static_cast<double>(v))); }

// the vector a missile flies by for a direction: g_GetSinCos(g_nCos / g_nSin, dir, 64), -1 outside 0..63
void dir_vector(int dir, int& vx, int& vy) noexcept
{
    vx = g_DirCos(dir);
    vy = g_DirSin(dir);
}

// the MoveKinds whose missiles take the direction's vector at creation (0x080EBD40, 0x080EBE88..)
bool takes_vector(int move_kind) noexcept
{
    return move_kind == missle_move_roll_back || move_kind == missle_move_line || move_kind == missle_move_follow || move_kind == missle_move_parabola;
}

// (dir << 6) >> 6 of the generators: the direction as the binary keeps it in +0x144 (an identity
// for every value the casts produce)
int norm_dir(int dir) noexcept { return dir; }

} // namespace

// ---- the table ------------------------------------------------------------------------------

KMissleTemplate KMissleTemplate::from_cells(const std::unordered_map<std::string, std::string>& cells)
{
    // 0x08074300, in the binary's order; every read with the default 0
    KMissleTemplate t;
    t.id = cell_int(cells, "MissleId");
    if (const auto it = cells.find("MissleName"); it != cells.end()) t.name = it->second;
    t.height = cell_int(cells, "MissleHeight") << 10;
    t.move_kind = cell_int(cells, "MoveKind");
    t.follow_kind = cell_int(cells, "FollowKind");
    t.life_time = cell_int(cells, "LifeTime");
    t.speed = cell_int(cells, "Speed");
    t.response_skill = cell_int(cells, "ResponseSkill");
    t.collide_range = cell_int(cells, "CollidRange");
    t.collide_vanish = (cell_int(cells, "ColVanish") & 0xff) != 0;
    t.range_damage = (cell_int(cells, "IsRangeDmg") & 0xff) != 0;
    t.damage_range = cell_int(cells, "DmgRange");
    t.z_acceleration = cell_int(cells, "Zacc");
    t.height_speed = cell_int(cells, "Zspeed");
    t.miss_rate = cell_int(cells, "MissRate");
    t.param1 = cell_int(cells, "Param1");
    t.param2 = cell_int(cells, "Param2");
    t.param3 = cell_int(cells, "Param3");
    t.auto_explode = (cell_int(cells, "AutoExplode") & 0xff) != 0;
    t.damage_interval = cell_int(cells, "DmgInterval");
    return t;
}

void KMissleTable::add(KMissleTemplate t)
{
    const int id = t.id;
    rows_[id] = std::move(t);
}

const KMissleTemplate* KMissleTable::find(int id) const
{
    const auto it = rows_.find(id);
    return it == rows_.end() ? nullptr : &it->second;
}

const KMissleTemplate* KMissleTable::basic_attack(int id)
{
    // missles.txt of the Linux server, rows 64 and 65 (the columns 0x08074300 reads)
    static const auto make = [](int missle_id) {
        std::unordered_map<std::string, std::string> cells = {
            {"MissleId", std::to_string(missle_id)}, {"MissleName", missle_id == 64 ? "Cong kich vat ly" : "Cong kich vat ly (xa)"},
            {"MoveKind", "1"}, {"FollowKind", "0"}, {"MissleHeight", "10"}, {"CollidRange", "1"}, {"IsRangeDmg", "0"}, {"DmgRange", "1"},
            {"DmgInterval", missle_id == 64 ? "6" : "0"}, {"LifeTime", missle_id == 64 ? "6" : "20"}, {"Speed", missle_id == 64 ? "20" : "16"},
            {"Zspeed", "0"}, {"Zacc", "0"}, {"ResponseSkill", "0"}, {"ColVanish", missle_id == 64 ? "0" : "1"}, {"AutoExplode", "0"},
            {"MissRate", "0"}, {"Param1", "0"}, {"Param2", "0"}, {"Param3", "0"},
        };
        return KMissleTemplate::from_cells(cells);
    };
    static const KMissleTemplate melee = make(64);
    static const KMissleTemplate ranged = make(65);
    if (id == 64) return &melee;
    if (id == 65) return &ranged;
    return nullptr;
}

std::optional<KMissleTable> KMissleTable::load(const std::string& file, std::string* error)
{
    auto fail = [&](std::string why) -> std::optional<KMissleTable> {
        if (error) *error = std::move(why);
        return std::nullopt;
    };
    std::ifstream in(file);
    if (!in) return fail("cannot open " + file);
    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        return fail(std::string("missles.json: ") + e.what());
    }
    KMissleTable table;
    try {
        const nlohmann::json rows = j.value("rows", nlohmann::json::array());
        for (const auto& r : rows) {
            if (!r.is_object()) continue;
            std::unordered_map<std::string, std::string> cells;
            if (const auto c = r.find("cells"); c != r.end() && c->is_object()) {
                for (const auto& [col, v] : c->items()) {
                    if (v.is_string()) cells[col] = v.get<std::string>();
                }
            }
            KMissleTemplate t = KMissleTemplate::from_cells(cells);
            const int id = r.value("id", t.id);
            // 0x0805D264: 1..999, every row written over the template in turn
            if (id < 1 || id > kMaxMissleTemplate) continue;
            t.id = id;
            table.add(std::move(t));
        }
    } catch (const std::exception& e) {
        return fail(std::string("missles.json: ") + e.what());
    }
    return table;
}

// ---- the set --------------------------------------------------------------------------------

int KSubWorld::missle_cells_x() const noexcept { return cfg_.map ? cfg_.map->cells_x : cfg_.width / kMissleCell; }
int KSubWorld::missle_cells_y() const noexcept { return cfg_.map ? cfg_.map->cells_y : cfg_.height / kMissleCell; }

const KMissleTemplate* KSubWorld::missle_template(int id) const noexcept
{
    if (cfg_.missles) return cfg_.missles->find(id);
    return KMissleTable::basic_attack(id);   // a map without the table (tests): the basic attacks still fly
}

const KSkill* KSubWorld::skill_of(int id, int level)
{
    // 0x08076CE0: m_pOrdinSkill[id][level] for id 1..1999 and level 1..63, InstanceSkill on a miss
    if (id < 1 || id > 1999 || level < 1 || level > 63 || skills_ == nullptr) return nullptr;
    return skills_->get(id, level);
}

bool KSubWorld::missle_set_pos_fine(KMissle& m, std::int64_t x1024, std::int64_t y1024) noexcept
{
    // Mps2Map 0x080EF7F0: the region of the point (-1 outside the map), the cell in it and the
    // rest as 1/1024 - here the absolute cell, the map's edge being the last region
    if (x1024 < 0 || y1024 < 0) {
        m.on_map = false;
        return false;
    }
    const auto cx = static_cast<int>(x1024 / kMissleCellUnits);
    const auto cy = static_cast<int>(y1024 / kMissleCellUnits);
    if (cx >= missle_cells_x() || cy >= missle_cells_y()) {
        m.on_map = false;
        return false;
    }
    m.map_x = cx;
    m.map_y = cy;
    m.x_offset = static_cast<int>(x1024 - static_cast<std::int64_t>(cx) * kMissleCellUnits);
    m.y_offset = static_cast<int>(y1024 - static_cast<std::int64_t>(cy) * kMissleCellUnits);
    m.on_map = true;
    return true;
}

bool KSubWorld::missle_set_pos(KMissle& m, Pos p) noexcept
{
    return missle_set_pos_fine(m, static_cast<std::int64_t>(p.x) << 10, static_cast<std::int64_t>(p.y) << 10);
}

KMissle* KSubWorld::missle_add(Pos at)
{
    // KMissleSet::Add 0x08076F00: a free slot (none: "no free missile" and -1), Mps2Map of the
    // point (a bad one: -1, the slot stays free), +0x74 = +0x1c, +0xf4 = the slot
    int idx = 0;
    if (!free_missles_.empty()) {
        idx = free_missles_.back();
        free_missles_.pop_back();
    } else {
        if (missles_.empty()) missles_.emplace_back();   // slot 0 is nobody's
        if (missles_.size() > static_cast<std::size_t>(kMaxMissles)) {
            log::warn("zone.fight", "no free missile", {log::kv("missles", live_missles_)});
            return nullptr;
        }
        idx = static_cast<int>(missles_.size());
        missles_.emplace_back();
    }
    KMissle& m = missles_[static_cast<std::size_t>(idx)];
    // the slot as its last user left it (0x08074CF0): the payload gone, the last collision cell -1;
    // everything the next CreateMissle does not write starts from nothing here
    m = KMissle{};
    if (!missle_set_pos(m, at)) {
        free_missles_.push_back(idx);
        return nullptr;
    }
    m.index = idx;
    m.map_z = m.height;
    ++live_missles_;
    return &m;
}

void KSubWorld::missle_remove(KMissle& m)
{
    // KMissleSet::Remove 0x08076E00 -> 0x08074CF0: the payload released, +0x160/+0x164 -1, the
    // series and the launcher's camp / PK mode cleared; the slot goes back to the free list
    if (!m.used()) return;
    const int idx = m.index;
    m.attribs.reset();
    m.last_map_x = m.last_map_y = -1;
    m.series = 0;
    m.launcher_camp = 0;
    m.launcher_pk_mode = 0;
    m.index = 0;
    free_missles_.push_back(idx);
    if (live_missles_ > 0) --live_missles_;
}

std::shared_ptr<const KMissleMagicAttribsList> KSubWorld::missle_attribs(const KSkill& skill, KNpc& launcher)
{
    // 0x080EA2D0 = KSkill::CreateMissleMagicAttribsData: null for a ClientSend skill; the list is
    // shared by every missile of the cast (the reference count at +0)
    KMissleMagicAttribsList list;
    if (!create_missle_magic_attribs_data(skill, launcher, list)) return nullptr;
    return std::make_shared<const KMissleMagicAttribsList>(std::move(list));
}

// ---- KSkill::CreateMissle 0x080EA310 ---------------------------------------------------------

void KSubWorld::create_missle(const KSkill& skill, KNpc& launcher, int missle_id, KMissle& m)
{
    if (missle_id < 1 || missle_id > kMaxMissleTemplate) return;   // 0x080EA328 (the slot and the launcher are valid here)
    // 0x08074AA0: the template's fields over the slot, the flight state cleared.  The array cell
    // of an id without a row is the zero template of the binary.
    const KMissleTemplate zero;
    const KMissleTemplate* t = missle_template(missle_id);
    if (t == nullptr) t = &zero;
    m.collide_event = false;
    m.collide_vanish = t->collide_vanish;
    m.range_damage = t->range_damage;
    m.follow_kind = t->follow_kind;
    m.move_kind = t->move_kind;
    m.angle = 0;                                    // +0x14c of the template
    m.collide_range = t->collide_range;
    m.damage_range = t->damage_range;
    m.height = t->height;
    m.life_time = t->life_time;
    m.speed = t->speed;
    m.param1 = t->param1;
    m.param2 = t->param2;
    m.param3 = t->param3;
    m.map_z = t->height >> 10;
    m.fly_event = false;                            // byte +0xa of the template
    m.fly_event_time = 0;                           // +0x54
    m.z_acceleration = t->z_acceleration;
    m.height_speed = t->height_speed;
    m.auto_explode = t->auto_explode;
    m.damage_interval = t->damage_interval;
    m.miss_rate = t->miss_rate;
    m.rest_hit_count = 0;                           // +0x158
    m.turned = false;
    m.turn_frame = 0;
    m.rel_x = m.rel_y = 0;
    m.des = Pos{};
    m.must_be_hit = false;
    m.arrive_from = m.arrive_to = 0;
    m.current_life = 0;
    m.follow = EntityId{};
    // 0x080EA36C: the skill's fields
    m.missle_id = missle_id;
    m.series = skill.row.series;
    m.level = skill.level;
    m.collide_event = skill.row.collide_event;
    m.vanished_event = skill.row.vanished_event;
    m.fly_event = skill.row.fly_event;
    m.fly_event_time = skill.row.fly_event_time;
    m.client_send = skill.row.client_send & 0xff;
    m.is_melee = skill.row.is_melee;
    m.target_self = skill.row.target_self;          // TargetSelf == 1
    m.interrupt_when_move = skill.row.stop_when_move;
    m.heel_at_parent = skill.row.heel_at_parent;
    m.use_attack_rating = skill.row.use_attack_rate;
    m.do_hurt = skill.row.do_hurt;
    m.relative_pos_type = skill.row.relative_pos_type;
    m.launcher_camp = launcher.current_camp & 0xff;
    m.launcher_pk_mode = 0;                         // KPlayer+0x5a50 of a player launcher (B3); 0 for a npc
    if (m.interrupt_when_move != 0) m.launcher_src = launcher.pos();
    m.relation = skill.row.relation;
    m.status = missle_status_wait;                  // 0x08074850
    // 0x080EA439: the missle_* attributes of the level
    for (int i = 0; i < skill.missle_attrib_count && i < kSkillAttribs; ++i) {
        const KMagicAttrib& a = skill.missle_attribs[static_cast<std::size_t>(i)];
        switch (a.type) {
        case magic_missle_movekind_v: m.move_kind = a.value[0]; break;
        case magic_missle_speed_v: m.speed = a.value[0]; break;
        case magic_missle_lifetime_v: m.life_time = a.value[0]; break;
        case magic_missle_height_v: m.height = a.value[0]; break;
        case magic_missle_damagerange_v: m.damage_range = a.value[0]; break;
        case magic_missle_missrate: m.miss_rate = a.value[0]; break;
        case magic_missle_hitcount: m.rest_hit_count = a.value[0]; break;
        case magic_missle_range:
            m.collide_range = a.value[0];
            m.range_damage = (a.value[1] & 0xff) != 0;
            m.damage_range = a.value[2];
            break;
        case magic_missle_dmginterval: m.damage_interval = a.value[0]; break;
        case magic_missle_zspeed:
            m.height_speed = a.value[0];
            m.z_acceleration = a.value[1];
            break;
        case magic_missle_ablility: m.collide_vanish = (a.value[1] & 0xff) != 0; break;
        case magic_missle_param:
            m.param1 = a.value[0];
            m.param2 = a.value[1];
            m.param3 = a.value[2];
            break;
        default: break;   // missle_radius_v and the reserves are not read
        }
    }
    // 0x080EA4E8: a melee missile keeps its life (0x08078DB0 hands the number back); any other
    // flies at half speed under slowmissle_b (0x08078DC0)
    if (!skill.row.is_melee && launcher.cur.slow_missle != 0) m.speed = half_toward_zero(m.speed);
    // 0x0807EE20: ColVanish stays; without it the launcher's +0x141c percent decides
    if (!m.collide_vanish) m.collide_vanish = launcher.cur.missle_vanish_rate > random(100);
    // 0x08079320: the state modifier of this skill on missle_missrate
    if (launcher.state_modifier.skill_id == skill.row.id && launcher.state_modifier.attrib == magic_missle_missrate) {
        m.miss_rate -= launcher.state_modifier.delta;
    }
}

// ---- the generators (KSkills.cpp) -------------------------------------------------------------

int KSubWorld::missle_start_life_time(const KSkill& skill, int i)
{
    // 0x080E8650 through the jump table 0x08258424 on MslsGenerate (a negative kind is > 5)
    const int data = skill.row.missles_generate_data;
    const int wait = skill.row.wait_time;
    switch (skill.row.missles_generate) {
    case 0: return wait;
    case 1: return data + wait;
    case 2: return data * i + wait;
    case 3:
        if (random(2) == 1) return i * data + wait + random(data);
        return i * data + wait - random(half_toward_zero(data));
    case 4: return wait + random(data);
    case 5: {
        const int n = skill.row.child_skill_num;
        if (n <= 1) return wait;
        return std::abs(i - n / 2) * data + wait;
    }
    default: return wait;
    }
}

KMissle* KSubWorld::missle_fire(const KSkill& skill, const KOrdinSkillParam& ctx, KNpc& launcher, Pos at, int dir, int dir_index, int i,
                                const std::shared_ptr<const KMissleMagicAttribsList>& list, const int* vector, bool vector_always)
{
    // the part every generator repeats (0x080EBBD0..0x080EBDB6 of CastWall): KMissleSet::Add,
    // the direction, CreateMissle, the target, the frame, the launcher, the parent, the skill, the
    // start delay, the birthplace, the vector for the kinds that fly by it, the payload
    KMissle* m = missle_add(at);
    if (m == nullptr) return nullptr;
    m->dir = dir;
    m->dir_index = dir_index;
    create_missle(skill, launcher, skill.row.child_skill_id, *m);
    m->follow = ctx.target;
    m->born_tick = tick_;
    m->launcher = launcher.id;
    m->parent_missle = ctx.parent_missle;
    m->skill_id = skill.row.id;
    m->start_life_time = missle_start_life_time(skill, i) + ctx.wait_time;
    m->life_time += m->start_life_time;
    m->ref = at;
    if (vector_always || takes_vector(m->move_kind)) {
        if (vector != nullptr) {
            m->x_factor = vector[0];
            m->y_factor = vector[1];
        } else {
            dir_vector(m->dir, m->x_factor, m->y_factor);
        }
    }
    m->attribs = list;
    return m;
}

int KSubWorld::cast_wall(const KSkill& skill, const KOrdinSkillParam& ctx, int dir, Pos at)
{
    // 0x080EBB20: ChildSkillNum missiles across the direction, Param1 apart, centred on `at`;
    // each flies along dir - 16 when Param2's low word is set, else along dir
    if (ctx.launcher_type != 0) return 0;
    KNpc* launcher = entities_.find(ctx.launcher);
    if (launcher == nullptr) return 0;
    const int spacing = skill.row.param1;
    const int n = skill.row.child_skill_num;
    const int dirn = norm_dir(dir);
    const auto list = missle_attribs(skill, *launcher);
    int count = 0;
    if (n <= 0) return count;
    int offset = half_toward_zero(-spacing * n);
    const int cos_d = g_DirCos(dirn);
    const int sin_d = g_DirSin(dirn);
    for (int i = 0; i < n; ++i) {
        const int px = at.x + ((cos_d * offset) >> 10);
        const int py = at.y + ((sin_d * offset) >> 10);
        if (py < 0 || px < 0) continue;   // 0x080EBDC0: the offset is not advanced either
        if (!skill.row.base_skill) {
            count += cast_child_skill(skill, ctx, px, py, i);
            offset += spacing;
            continue;
        }
        int mdir = dir;
        int mdir_index = dirn;
        if ((skill.row.param2 & 0xffff) != 0) {   // 0x080EBC1D: word [+0x54]
            mdir = dirn - 16 >= 0 ? dirn - 16 : dirn + 48;
            mdir_index = mdir;
        }
        if (missle_fire(skill, ctx, *launcher, Pos{px, py}, mdir, mdir_index, i, list, nullptr, false) == nullptr) continue;   // no slot: no advance
        ++count;
        offset += spacing;
    }
    return count;
}

int KSubWorld::cast_line(const KSkill& skill, const KOrdinSkillParam& ctx, int dir, Pos at)
{
    // 0x080EC2F0: ChildSkillNum missiles along the direction, the i-th Param1 x (i + 1) from `at`
    if (ctx.launcher_type != 0) return 0;
    KNpc* launcher = entities_.find(ctx.launcher);
    if (launcher == nullptr) return 0;
    const int spacing = skill.row.param1;
    const int dirn = norm_dir(dir);
    const int step_x = g_DirCos(dirn) * spacing;
    const int step_y = g_DirSin(dirn) * spacing;
    const auto list = missle_attribs(skill, *launcher);
    const int n = skill.row.child_skill_num;
    int count = 0;
    int acc_x = step_x;
    int acc_y = step_y;
    for (int i = 0; i < n; ++i, acc_x += step_x, acc_y += step_y) {
        const int px = at.x + (acc_x >> 10);
        const int py = at.y + (acc_y >> 10);
        if (py < 0 || px < 0) continue;
        if (!skill.row.base_skill) {
            count += cast_child_skill(skill, ctx, px, py, i);
            continue;
        }
        if (missle_fire(skill, ctx, *launcher, Pos{px, py}, dir, dirn, i, list, nullptr, false) == nullptr) continue;
        ++count;
    }
    return count;
}

int KSubWorld::cast_extractive_line_missle(const KSkill& skill, const KOrdinSkillParam& ctx, int dir, Pos from, int ux, int uy, Pos to)
{
    // 0x080EBF00: one missile at the launcher that must reach the spot `to` (a MoveKind 7 child):
    // it flies the unit vector (ux, uy) and can only hit in the frames it should be there
    if (ctx.launcher_type != 0) return 0;
    KNpc* launcher = entities_.find(ctx.launcher);
    if (launcher == nullptr) return 0;
    const auto list = missle_attribs(skill, *launcher);
    if (!skill.row.base_skill) return cast_child_skill(skill, ctx, 0, 0, 0);   // 0x080EC1F0: at (0, 0), the first
    KMissle* m = missle_add(from);
    if (m == nullptr) return 0;
    m->dir = dir;
    m->dir_index = norm_dir(dir);
    create_missle(skill, *launcher, skill.row.child_skill_id, *m);
    const std::int64_t dx = from.x - to.x;
    const std::int64_t dy = from.y - to.y;
    const std::int64_t dist2 = dx * dx + dy * dy;
    if (m->move_kind == missle_move_parabola) {   // 0x080EC263: the climb that lands it there
        const int len = int_sqrt(dist2);
        if (m->speed != 0) {
            const int frames = len / m->speed;
            m->height_speed = half_toward_zero((frames - 1) * m->z_acceleration);
        }
    }
    m->follow = ctx.target;
    m->born_tick = tick_;
    m->launcher = launcher->id;
    m->parent_missle = ctx.parent_missle;
    m->skill_id = skill.row.id;
    m->start_life_time = missle_start_life_time(skill, 0) + ctx.wait_time;
    m->life_time += m->start_life_time;
    m->must_be_hit = true;
    m->ref = from;
    const int len = int_sqrt(dist2);
    if (m->speed == 0) return 0;   // 0x080EC113: the missile stays, without its payload
    m->arrive_from = m->start_life_time + len / m->speed;
    m->arrive_to = m->arrive_from + kMissleCell / m->speed + 2;
    if (takes_vector(m->move_kind)) {
        m->x_factor = ux;
        m->y_factor = uy;
    }
    m->attribs = list;
    return 1;
}

int KSubWorld::cast_spread(const KSkill& skill, const KOrdinSkillParam& ctx, int dir, Pos at)
{
    // 0x080EB150: a fan of ChildSkillNum missiles Param2 away from `at`, Param1 directions apart,
    // aimed at the target when there is one (the unit vector to it turned by the fan angle)
    if (ctx.launcher_type != 0) return 0;
    KNpc* launcher = entities_.find(ctx.launcher);
    if (launcher == nullptr) return 0;
    const int dist = skill.row.param2;
    const auto list = missle_attribs(skill, *launcher);
    int ux = 0;
    int uy = 0;
    const KNpc* target = ctx.target.valid() ? entities_.find(ctx.target) : nullptr;
    if (target != nullptr) {   // 0x080EB1BC (a target the set no longer holds reads garbage there; none here)
        const Pos tp = target->pos();
        const std::int64_t dx = tp.x - at.x;
        const std::int64_t dy = tp.y - at.y;
        const int len = int_sqrt(dx * dx + dy * dy);
        if (len == 0) {
            ux = g_DirCos(dir);
            uy = g_DirSin(dir);
        } else {
            ux = static_cast<int>((dx << 10) / len);
            uy = static_cast<int>((dy << 10) / len);
        }
        if (at.y + ((uy * dist) >> 10) < 0) return 0;
        if (at.x + ((ux * dist) >> 10) < 0) return 0;
    }
    const int n = skill.row.child_skill_num;
    int count = 0;
    if (n <= 0) return count;
    int half = half_toward_zero(n);
    for (int i = 0; i < n; ++i) {
        int b = dir - half * skill.row.param1;
        if (b < 0) b += 64;
        else if (b >= 64) b -= 64;
        int vx = 0;
        int vy = 0;
        if (target != nullptr) {
            const int hp = half * skill.row.param1;
            const int a = hp + 48 >= 64 ? hp - 16 : hp + 48;
            const int ca = g_DirCos(a);
            const int sa = g_DirSin(a);
            vy = (ca * uy - sa * ux) >> 10;
            vx = (sa * uy + ca * ux) >> 10;
        } else {
            vy = g_DirSin(b);
            vx = g_DirCos(b);
        }
        const int px = at.x + ((dist * vx) >> 10);
        const int py = at.y + ((dist * vy) >> 10);
        if (py < 0 || px < 0) continue;
        if (!skill.row.base_skill) {
            count += cast_child_skill(skill, ctx, at.x, at.y, i);   // 0x080EB618: at the origin
            --half;
            continue;
        }
        const int vec[2] = {vx, vy};
        if (missle_fire(skill, ctx, *launcher, Pos{px, py}, b, norm_dir(b), i, list, vec, true) == nullptr) continue;   // no slot: half stays
        ++count;
        --half;
    }
    return count;
}

int KSubWorld::cast_circle(const KSkill& skill, const KOrdinSkillParam& ctx, int dir, Pos at)
{
    // 0x080EB720: ChildSkillNum missiles on a ring of radius Param2 around `at`, 64 / n directions
    // apart starting at dir, each flying outwards
    if (ctx.launcher_type != 0) return 0;
    KNpc* launcher = entities_.find(ctx.launcher);
    if (launcher == nullptr) return 0;
    const int n = skill.row.child_skill_num;
    const int radius = skill.row.param2;
    if (n <= 0) return 0;
    const int step = 64 / n;
    const auto list = missle_attribs(skill, *launcher);
    int count = 0;
    int angle = dir;
    for (int i = 0; i < n; ++i, angle += step) {
        const int d = angle < 0 ? angle + 64 : (angle >= 64 ? angle - 64 : angle);
        const int px = at.x + ((g_DirCos(d) * radius) >> 10);
        const int py = at.y + ((g_DirSin(d) * radius) >> 10);
        if (py < 0 || px < 0) continue;
        if (!skill.row.base_skill) {
            count += cast_child_skill(skill, ctx, px, py, i);
            continue;
        }
        if (missle_fire(skill, ctx, *launcher, Pos{px, py}, d, norm_dir(d), i, list, nullptr, false) == nullptr) continue;
        ++count;
    }
    return count;
}

int KSubWorld::cast_zone(const KSkill& skill, const KOrdinSkillParam& ctx, int dir, Pos at)
{
    // 0x080EC690: a ChildSkillNum x ChildSkillNum grid of cells centred on `at` (a single one at
    // `at` itself); with Param1 == 1 only the cells inside the circle
    if (ctx.launcher_type != 0) return 0;
    KNpc* launcher = entities_.find(ctx.launcher);
    if (launcher == nullptr) return 0;
    const int n = skill.row.child_skill_num;
    Pos o = at;
    if (n != 1) {
        o.x = at.x - half_toward_zero(kMissleCell * n);
        o.y = at.y - half_toward_zero(kMissleCell * n);
    }
    const auto list = missle_attribs(skill, *launcher);
    int count = 0;
    if (n <= 0) return count;
    const int dirn = norm_dir(dir);
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            if (!skill.row.base_skill) {
                count += cast_child_skill(skill, ctx, o.x + kMissleCell * i, o.y + kMissleCell * j, j * n + i);
                continue;
            }
            if (skill.row.param1 == 1) {   // 0x080ECA48
                const int c = half_toward_zero(n);
                const int dj = j - c;
                const int di = i - c;
                if (dj * dj + di * di > quarter_toward_zero(n * n)) continue;
            }
            const Pos p{o.x + kMissleCell * i, o.y + kMissleCell * j};
            if (missle_fire(skill, ctx, *launcher, p, dir, dirn, j * n + i, list, nullptr, false) == nullptr) continue;
            ++count;
        }
    }
    return count;
}

int KSubWorld::cast_child_skill(const KSkill& skill, const KOrdinSkillParam& ctx, int px, int py, int i)
{
    // 0x080EAFF0: a cast that is not a BaseSkill fires its ChildSkillId (at ChildSkillLevel, -1 =
    // this level) at the spot instead of a missile, delayed like the missile would have been; by
    // the parent missile when the cast came from one
    int level = skill.row.child_skill_level;
    if (level == -1) level = skill.level;
    if (level <= 0) return 0;
    const int child = skill.row.child_skill_id;
    if (child <= 0 || child > 1999 || level > 63) return 0;
    const KSkill* sk = skills_ ? skills_->get(child, level) : nullptr;
    if (sk == nullptr) return 0;
    KCastParams p;
    p.at_pos = true;
    p.pos = Pos{px, py};
    p.wait_time = missle_start_life_time(skill, i) + ctx.wait_time;
    if (ctx.parent_missle != 0) {
        if (ctx.parent_type != 2) {
            // the CastWall quirk (0x080ED5EC): a wall cast by a missile writes 2 where the parent
            // index goes, so the binary would cast this child through npc #2 - not carried
            log::trace("zone.fight", "child cast of a wall by a missile refused", {log::kv("skill", skill.row.id), log::kv("child", child)});
            return 0;
        }
        return missle_skill_cast(*sk, ctx.parent_missle, p) ? 1 : 0;
    }
    KNpc* launcher = entities_.find(ctx.launcher);
    if (launcher == nullptr) return 0;
    return skill_cast(*sk, *launcher, p) ? 1 : 0;
}

EntityId KSubWorld::cast_target_position(const KCastParams& p, Pos& out) const
{
    // 0x080EED70: a cast on a npc takes its spot and the npc; a cast at a spot takes the centre of
    // its cell and no npc; a cast in a direction leaves the spot as it was (0, 0) and no npc
    if (p.dir >= 0) return EntityId{};
    if (!p.at_pos) {
        const KNpc* t = entities_.find(p.target);
        out = t != nullptr ? t->pos() : Pos{};
        return p.target;
    }
    auto snap = [](int v) { return ((v < 0 ? v + 31 : v) & ~31) + 16; };
    out = Pos{snap(p.pos.x), snap(p.pos.y)};
    if (out.x < 0 || out.y < 0) out = Pos{};   // 0x080EEDD2 (logged there)
    return EntityId{};
}

int KSubWorld::cast_missles(const KSkill& skill, KMissle* by, KNpc& launcher, const KCastParams& p)
{
    // KSkill::CastMissles 0x080ECAC0.  eLauncherType 0 is the npc `launcher`; 2 is the missile
    // `by`, whose npc the caller resolved (0x080ECD21: the missile's launcher id must still be its
    // npc's).  The jump table 0x08258478 on MisslesForm picks the generator.
    if (skill.row.client_send == 2) return 1;
    KOrdinSkillParam ctx;
    ctx.launcher = launcher.id;
    ctx.wait_time = p.wait_time;
    ctx.target = p.target;
    const int form = skill.row.missles_form;
    const bool by_missle = by != nullptr;
    if (form < 0 || form > missles_form_at_firer) {   // > 7 (unsigned): the start event alone
        skill_start_event(skill, launcher, p);
        return 1;
    }
    const Pos lp = launcher.pos();
    const Pos mp = by_missle ? missle_pos(*by) : Pos{};
    Pos tp{};
    auto aim = [&](EntityId& target, int& dir) {
        target = cast_target_position(p, tp);
        dir = g_GetDirIndex(lp.x, lp.y, tp.x, tp.y);
        ctx.target = target;
    };
    auto from_missle = [&](int parent_type) {
        ctx.parent_missle = by->index;
        ctx.parent_type = parent_type;
        ctx.target = by->follow;
    };
    EntityId target;
    int dir = -1;
    switch (form) {
    case missles_form_wall:   // 0x080ECBD8
        if (p.dir >= 0) return 0;
        if (by_missle) {
            ctx.parent_missle = 2;   // sic: the binary writes 2 where the parent index goes (0x080ED5EC)
            ctx.parent_type = 0;
            ctx.target = by->follow;
            const int d = by->dir + 16 >= 64 ? by->dir - 48 : by->dir + 16;
            cast_wall(skill, ctx, d, mp);
        } else {
            aim(target, dir);
            const int d = dir + 16 >= 64 ? dir - 48 : dir + 16;
            if (((skill.row.param2 >> 16) & 0xffff) != 0) cast_wall(skill, ctx, d, lp);   // word [+0x56]
            else cast_wall(skill, ctx, d, tp);
        }
        break;
    case missles_form_line:   // 0x080ECDE0
        if (p.dir >= 0) {
            if (p.dir > 64) return 0;
            if (by_missle) {   // 0x080EDB00: through CastWall (the binary's copy of the branch)
                ctx.parent_missle = by->index;
                ctx.target = by->follow;
                cast_wall(skill, ctx, p.dir, mp);
            } else {
                ctx.target = EntityId{};
                cast_line(skill, ctx, p.dir, lp);
            }
        } else if (by_missle) {
            from_missle(2);
            cast_line(skill, ctx, by->dir, mp);
        } else {
            aim(target, dir);
            const KMissleTemplate* child = missle_template(skill.row.child_skill_id);
            if (child != nullptr && child->move_kind == missle_move_parabola) {   // 0x080ECEC8
                if (lp == tp) return 0;
                const std::int64_t dx = lp.x - tp.x;
                const std::int64_t dy = lp.y - tp.y;
                const int len = int_sqrt(dx * dx + dy * dy);
                if (len == 0) return 0;
                const int uy = static_cast<int>((static_cast<std::int64_t>(tp.y - lp.y) << 10) / len);
                if (std::abs(uy) > 1024) return 0;
                const int ux = static_cast<int>((static_cast<std::int64_t>(tp.x - lp.x) << 10) / len);
                if (std::abs(ux) > 1024) return 0;
                cast_extractive_line_missle(skill, ctx, dir, lp, ux, uy, tp);
            } else {
                cast_line(skill, ctx, dir, lp);
            }
        }
        break;
    case missles_form_spread:   // 0x080ECCF8
        if (p.dir >= 0) {
            if (p.dir > 64) return 0;
            if (by_missle) {
                from_missle(2);
                cast_spread(skill, ctx, p.dir, mp);
            } else {
                ctx.target = EntityId{};
                cast_spread(skill, ctx, p.dir, lp);
            }
        } else if (by_missle) {
            from_missle(2);
            cast_spread(skill, ctx, by->dir, mp);
        } else {
            aim(target, dir);
            cast_spread(skill, ctx, dir, lp);
        }
        break;
    case missles_form_circle:   // 0x080ED258
        if (p.dir >= 0) return 0;
        if (by_missle) {
            from_missle(2);
            cast_circle(skill, ctx, by->dir, mp);
        } else {
            aim(target, dir);
            if (skill.row.param1 != 0) cast_circle(skill, ctx, dir, tp);
            else cast_circle(skill, ctx, dir, lp);
        }
        break;
    case missles_form_random:   // 0x080ECB4C: nothing but the start event
        break;
    case missles_form_zone:   // 0x080ED0A0: at the launcher, the target kept
        if (p.dir >= 0) return 0;
        if (by_missle) {
            from_missle(2);
            cast_zone(skill, ctx, by->dir, mp);
        } else {
            aim(target, dir);
            cast_zone(skill, ctx, dir, lp);
        }
        break;
    case missles_form_at_target:   // 0x080ED018
        if (p.dir >= 0) return 0;
        if (by_missle) {
            from_missle(2);
            cast_zone(skill, ctx, by->dir, mp);
        } else {
            aim(target, dir);
            cast_zone(skill, ctx, dir, tp);
        }
        break;
    case missles_form_at_firer:   // 0x080ED188: at the launcher, no target (the direction is toward (0, 0))
        if (p.dir >= 0) return 0;
        if (by_missle) {
            from_missle(2);
            cast_zone(skill, ctx, by->dir, mp);
        } else {
            dir = g_GetDirIndex(lp.x, lp.y, 0, 0);
            ctx.target = EntityId{};
            cast_zone(skill, ctx, dir, lp);
        }
        break;
    default:
        break;
    }
    skill_start_event(skill, launcher, p);   // 0x080ECB50 / 0x080ED371 (with the missile's npc)
    return 1;
}

int KSubWorld::cast_instant_missle(const KSkill& skill, KNpc& launcher, const KCastParams& p)
{
    // 0x080EA720 (style 14): the start event first, then one missile of the child template at the
    // target's spot (or the spot given, unsnapped) that deals its area blow and vanishes at once;
    // it aims at nobody
    if (skill.row.child_skill_id <= 0) return 0;
    Pos at;
    if (p.dir >= 0) return 0;   // (-2, dir) is no place: KMissleSet::Add fails on it
    if (p.at_pos) {
        at = p.pos;
    } else {
        const KNpc* t = entities_.find(p.target);
        if (t == nullptr) return 0;
        at = t->pos();
    }
    skill_start_event(skill, launcher, p);
    const auto list = missle_attribs(skill, launcher);
    if (!list) return 0;
    KMissle* m = missle_add(at);
    if (m == nullptr) return 0;
    create_missle(skill, launcher, skill.row.child_skill_id, *m);
    m->born_tick = tick_;
    m->launcher = launcher.id;
    m->attribs = list;
    m->skill_id = skill.row.id;
    m->ref = at;
    missle_process_collision(*m);
    missle_do_vanish(*m, true);
    return 1;
}

bool KSubWorld::missle_skill_cast(const KSkill& skill, int missle_index, const KCastParams& p)
{
    // KSkill::Cast 0x080EA920 with eLauncherType 2: the missile must be in use, a target must be
    // on this map, then the style decides - only CastMissles takes a missile launcher
    KMissle* by = missle_index > 0 && static_cast<std::size_t>(missle_index) < missles_.size() ? &missles_[static_cast<std::size_t>(missle_index)] : nullptr;
    if (by == nullptr || !by->used()) return false;
    if (!p.at_pos && p.dir < 0 && entities_.find(p.target) == nullptr) return false;
    KCastParams q = p;
    if (q.wait_time < 0) {
        log::trace("zone.fight", "cast wait below zero", {log::kv("entity", by->launcher), log::kv("skill", skill.row.id)});
        q.wait_time = 0;
    }
    if (skill.row.style > skill_style_jx2_14) return true;
    if (skill.row.style == skill_style_missles) {
        KNpc* launcher = entities_.find(by->launcher);
        if (launcher == nullptr) return false;   // 0x080ECD3A: the missile's npc must still be the one it was fired by
        cast_missles(skill, by, *launcher, q);
        return true;
    }
    if (skill.row.style == skill_style_melee || (skill.row.style >= 5 && skill.row.style <= 13)) return true;
    log::trace("zone.fight", "cast by a missile not carried", {log::kv("missle", by->index), log::kv("skill", skill.row.id), log::kv("style", skill.row.style)});
    return true;
}

// ---- the frame ---------------------------------------------------------------------------------

void KSubWorld::activate_missles()
{
    // KMissleSet::Activate 0x08076EB0: every slot in turn (a missile fired into a later slot
    // during the walk gets its first frame now, like the binary's); then the removals
    for (std::size_t i = 1; i < missles_.size(); ++i) {
        KMissle& m = missles_[i];
        if (m.used()) missle_frame(m);
    }
    for (std::size_t i = 1; i < missles_.size(); ++i) {
        KMissle& m = missles_[i];
        if (!m.used()) continue;
        if (m.status == missle_status_vanished) {
            missle_remove(m);
        } else if (!m.on_map) {
            // a missile HeelAtParent moved off the map sits in the binary with region -1 for good
            log::trace("zone.fight", "missile off the map", {log::kv("missle", m.index), log::kv("skill", m.skill_id)});
            missle_remove(m);
        }
    }
}

void KSubWorld::missle_frame(KMissle& m)
{
    // 0x08076950: the launcher must be the npc it was (a player also in the same camp and PK
    // mode), a target that is gone is forgotten, then the life, the start and the flight
    if (!m.used() || !m.on_map) return;
    KNpc* launcher = entities_.find(m.launcher);
    if (launcher == nullptr) {
        missle_do_vanish(m, false);
        return;
    }
    if (launcher->kind == KNpcKind::player) {
        if ((launcher->current_camp & 0xff) != m.launcher_camp) {
            missle_do_vanish(m, false);
            return;
        }
        if (0 != m.launcher_pk_mode) {   // KPlayer+0x5a50 (B3)
            missle_do_vanish(m, false);
            return;
        }
    }
    if (m.follow.valid() && entities_.find(m.follow) == nullptr) m.follow = EntityId{};
    const int frame = m.current_life;
    int status = m.status;
    if (frame >= m.life_time) {   // 0x08076A0B
        if (status != missle_status_vanished && status != 3) {
            if (m.auto_explode) {
                missle_process_collision(m);
                if (m.collide_event) {
                    if (const KSkill* sk = skill_of(m.skill_id, m.level)) missle_event(*sk, 3, m);
                }
            }
            missle_do_vanish(m, true);
            ++m.current_life;
            return;
        }
    }
    if (frame == m.start_life_time) {   // 0x08076AB8
        if (status == missle_status_vanished) {
            ++m.current_life;
            return;
        }
        if (!missle_prepare_fly(m)) {
            missle_do_vanish(m, true);
            ++m.current_life;
            return;
        }
        m.status = missle_status_fly;
    } else if (status != missle_status_fly) {
        ++m.current_life;
        return;
    }
    missle_activate(m);
    if (m.fly_event) {   // 0x08076AE8: FlySkillId every FlyEventTime frames of flight
        if (m.fly_event_time > 0 && (m.current_life - m.start_life_time) % m.fly_event_time == 0) {
            if (m.level <= 0) return;   // 0x08076B12: without counting the frame
            if (const KSkill* sk = skill_of(m.skill_id, m.level)) missle_event(*sk, 2, m);
        }
    }
    ++m.current_life;
}

void KSubWorld::missle_activate(KMissle& m)
{
    // KMissle::Activate 0x080760E0
    if (m.interrupt_when_move == 2) {   // 0x080760F3: gone once the launcher moved
        const KNpc* l = entities_.find(m.launcher);
        if (l == nullptr || l->pos() != m.launcher_src) {
            missle_do_vanish(m, true);
            return;
        }
    }
    if (m.z_acceleration != 0) {   // ZAxisMove
        m.height += m.height_speed;
        if (m.height < 0) m.height = 0;
        m.map_z = m.height >> 10;
        m.height_speed -= m.z_acceleration;
    }
    // 0x08076126: with a DmgInterval the collisions are only looked for once it is due
    bool due = true;
    bool interval_pending = false;
    if (m.damage_interval != 0 && m.next_damage_tick >= tick_) {
        due = false;
        interval_pending = true;
    }
    if (m.move_kind == missle_move_line) {   // 0x08076247: in steps of 10 units
        const int n = m.speed / kMissleFlyStep;
        bool check = due;
        for (int i = 0; i < n; ++i) {
            const int step_x = m.map_x;
            int step_y = m.map_y;
            bool c = check;
            if (m.last_map_x == m.map_x) {
                if (m.last_map_y == m.map_y) {
                    check = false;   // 0x08076420: from now on, until a cell changes
                    c = false;
                }
            }
            step_y = m.map_y;
            const int r = missle_on_fly(m, kMissleFlyStep, c);
            if (r == 1) {
                missle_do_collision(m);
                if (m.status != missle_status_fly) return;
            } else if (r == 2) {
                missle_do_vanish(m, true);
                return;
            }
            // 0x0807632F: the next step looks again only if this one crossed into another cell
            check = false;
            if (!interval_pending) {
                if (step_x == m.map_x) check = m.map_y != step_y;
                else check = true;
            }
        }
        const int r = missle_on_fly(m, m.speed % kMissleFlyStep, check);   // 0x080763AA: the rest
        if (r == 1) {
            missle_do_collision(m);
            return;
        }
        if (r == 2) missle_do_vanish(m, true);
        return;
    }
    bool check = due;
    if (m.move_kind != missle_move_stand && m.last_map_x == m.map_x && m.last_map_y == m.map_y) check = false;   // 0x08076520
    int r = missle_on_fly(m, m.speed, check);
    if (r == 1) {   // 0x080764A0: the blow, then the move without a check
        missle_do_collision(m);
        if (m.status != missle_status_fly) return;
        r = missle_on_fly(m, m.speed, false);
    }
    if (r == 2) missle_do_vanish(m, true);
}

bool KSubWorld::missle_prepare_fly(KMissle& m)
{
    // PrePareFly 0x08076550
    if (m.move_kind == missle_move_roll_back) {   // 0x08076650: it turns half way through its life
        m.turn_frame = half_toward_zero(m.life_time - m.start_life_time) + m.start_life_time;
    } else if (m.move_kind == missle_move_circle && m.param2 == 0) {   // 0x08076670: it starts above the launcher
        if (const KNpc* l = entities_.find(m.launcher)) {
            const Pos lp = l->pos();
            int y = lp.y - kMissleCircleRadius - m.speed;
            if (y < 0) y = 0;
            // Mps2Map of the spot; the binary keeps the old spot when the region is -1 - or 0
            // (0x0807670E: "jle"), the top-left region of the map
            const Pos p{lp.x, y};
            const bool region_zero = p.x >= 0 && p.y >= 0 && p.x < kMissleCell * 16 && p.y < kMissleCell * 32;
            if (!region_zero) {
                KMissle probe = m;
                if (missle_set_pos(probe, p)) {
                    m.map_x = probe.map_x;
                    m.map_y = probe.map_y;
                    m.x_offset = probe.x_offset;
                    m.y_offset = probe.y_offset;
                }
            }
        }
    }
    if (m.interrupt_when_move != 0) {   // 0x080765C8
        const KNpc* l = entities_.find(m.launcher);
        if (l == nullptr || l->pos() != m.launcher_src) return false;
    }
    if (m.heel_at_parent) {   // 0x08076578: born relative to its parent's spot now
        Pos parent;
        if (m.parent_missle == 0) {
            const KNpc* l = entities_.find(m.launcher);
            if (l == nullptr) return false;
            parent = l->pos();
        } else {
            const KMissle* pm = missle(m.parent_missle);
            if (pm == nullptr || pm->launcher != m.launcher) return false;
            parent = missle_pos(*pm);
        }
        const Pos own = missle_pos(m);
        missle_set_pos(m, Pos{own.x + parent.x - m.ref.x, own.y + parent.y - m.ref.y});   // off the map: region -1, kept (activate_missles)
        return true;
    }
    return true;
}

int KSubWorld::barrier_kind(Pos at) const noexcept
{
    // KRegion::GetBarrier 0x080E0A30(region, mode, cx, cy, ox, oy) after Mps2Map (0x080F0530 /
    // 0x080F05C0): the cell's dword - its low byte is what obstacle.bin keeps - has the barrier
    // kind in the low nibble and, in the high nibble, a diagonal shape: 3 passes where ox < oy, 4
    // where ox > oy, 5 where ox + oy <= 31, 2 where ox + oy > 32 (ox, oy = the offsets inside the
    // cell in units).  Where it passes, or with no shape and kind 0, the answer is 0 - or 4 when a
    // npc stands in the cell and the caller's mode asks (the missiles ask and take 4 for nothing;
    // the knock back's mode is 0 and the global 0x0830CA2C is not read here: neither asks).
    if (!cfg_.map) return 0;
    const KMapData& map = *cfg_.map;
    if (at.x < 0 || at.y < 0) return -1;
    const int cx = at.x / kMissleCell;
    const int cy = at.y / kMissleCell;
    if (!map.in_bounds(cx, cy)) return -1;
    const int cell = map.obstacle[static_cast<std::size_t>(cy) * static_cast<std::size_t>(map.cells_x) + static_cast<std::size_t>(cx)];
    const int shape = (cell >> 4) & 0xf;
    const int ox = at.x - cx * kMissleCell;
    const int oy = at.y - cy * kMissleCell;
    bool passes = false;
    switch (shape) {
    case 3: passes = ox < oy; break;
    case 4: passes = ox > oy; break;
    case 5: passes = ox + oy <= 31; break;
    case 2: passes = ox + oy > 32; break;
    default: break;
    }
    if (passes) return 0;
    return cell & 0xf;
}

bool KSubWorld::missle_test_barrier(const KMissle& m) const noexcept
{
    // KMissle::TestBarrier: Obstacle_Normal (1) or Obstacle_Jump (3) under the missile (0x080F05C0
    // resolves the cell across regions; -1 off the map is not a barrier)
    if (!cfg_.map || !m.on_map) return false;
    const int kind = barrier_kind(missle_pos(m));
    return kind == 1 || kind == 3;
}

int KSubWorld::missle_on_fly(KMissle& m, int speed, bool check)
{
    // KMissle::OnFly 0x080758E0
    if (speed == 0 && m.move_kind != missle_move_stand) return 0;
    if (missle_test_barrier(m)) return 2;
    if (check) {   // 0x08075C70
        const int r = missle_check_collision(m);
        if (r == -1) {   // the ground
            if (m.auto_explode) {
                missle_process_collision(m);
                if (m.collide_event) {
                    if (const KSkill* sk = skill_of(m.skill_id, m.level)) missle_event(*sk, 3, m);
                }
            }
            return 2;
        }
        if (r == 1) return 1;
    }
    if (m.status != missle_status_fly) return 2;
    int dx = 0;
    int dy = 0;
    switch (m.move_kind) {
    case missle_move_line:
    case missle_move_parabola:   // 0x08075D38
        dx = m.x_factor * speed;
        dy = m.y_factor * speed;
        break;
    case missle_move_circle: {   // 0x080759AF: a chord of the circle of radius speed + 50 per frame
        const int a = m.angle;
        const int prev = a - 1 < 0 ? 63 : a - 1;
        m.dir = a + 16 > 63 ? a - 48 : a + 16;
        const int r = speed + kMissleCircleRadius;
        dx = (g_DirCos(a) - g_DirCos(prev)) * r;
        dy = (g_DirSin(a) - g_DirSin(prev)) * r;
        if (m.param1 != 0) m.angle = a + 1 > 63 ? 0 : a + 1;
        else m.angle = a - 1 < 0 ? 63 : a - 1;
        break;
    }
    case missle_move_helix: {   // 0x08075AA0: the radius grows with the frames; around the launcher when Param2 == 0
        const int a = m.angle;
        const int prev = a - 1 < 0 ? 63 : a - 1;
        m.dir = a + 16 > 63 ? a - 48 : a + 16;
        const int r = speed + m.current_life + kMissleCircleRadius;
        dx = (g_DirCos(a) - g_DirCos(prev)) * r;
        dy = (g_DirSin(a) - g_DirSin(prev)) * r;
        if (m.param2 == 0) {
            if (const KNpc* l = entities_.find(m.launcher)) missle_set_pos_fine(m, npc_x1024(*l), npc_y1024(*l));
        }
        if (m.param1 != 0) m.angle = a + 1 > 63 ? 0 : a + 1;
        else m.angle = a - 1 < 0 ? 63 : a - 1;
        break;
    }
    case missle_move_follow: {   // 0x08075D58: every ninth frame it aims at the target again
        const int counted = m.param1;
        m.param1 = counted + 1;
        if (counted > 7) {
            m.param1 = 0;
            if (const KNpc* t = m.follow.valid() ? entities_.find(m.follow) : nullptr) {
                const Pos mp = missle_pos(m);
                m.des = t->pos();
                const int cx = m.des.x - mp.x;
                const int cy = m.des.y - mp.y;
                const int len = int_sqrt(static_cast<std::int64_t>(cx) * cx + static_cast<std::int64_t>(cy) * cy);
                if (len != 0) {
                    m.x_factor = (cx << 10) / len;
                    m.y_factor = (cy << 10) / len;
                    m.param2 = len / speed + 1;
                    const int d = g_GetDirIndex(mp.x, mp.y, m.des.x, m.des.y);
                    m.dir = d;
                    m.dir_index = d;
                }
            }
        }
        if (m.param2 != 0 && --m.param2 == 0) {   // 0x08075DA0: the last frame jumps onto the spot
            const Pos mp = missle_pos(m);
            dx = (m.des.x - mp.x) << 10;
            dy = (m.des.y - mp.y) << 10;
        } else {
            dx = m.x_factor * speed;
            dy = m.y_factor * speed;
        }
        break;
    }
    case missle_move_roll_back: {   // 0x08075CC8: it comes back at its turn frame
        if (!m.turned && m.turn_frame <= m.current_life) {
            m.turned = true;
            m.x_factor = -m.x_factor;
            m.y_factor = -m.y_factor;
            m.dir = m.dir - 32 < 0 ? m.dir + 32 : m.dir - 32;
        }
        dx = m.x_factor * speed;
        dy = m.y_factor * speed;
        break;
    }
    default:   // 0 stand, 2, 6, 8..99, > 100: no move (0x08075CBB)
        break;
    }
    return missle_check_beyond_region(m, dx, dy) ? 0 : 2;
}

bool KSubWorld::missle_relative_base(const KMissle& m, Pos& out) const
{
    // 0x08074D70: the anchor a RelativePosType missile flies from
    out = Pos{};
    switch (m.relative_pos_type) {
    case 1: out = m.ref; return true;
    case 2: {
        const KNpc* l = entities_.find(m.launcher);
        if (l == nullptr) return false;
        out = l->pos();
        return true;
    }
    case 3: {
        const KNpc* t = m.follow.valid() ? entities_.find(m.follow) : nullptr;
        if (t == nullptr) return false;
        out = t->pos();
        return true;
    }
    case 4: {
        if (m.parent_missle > 0) {
            if (const KMissle* pm = missle(m.parent_missle)) {
                out = missle_pos(*pm);
                return true;
            }
            return false;
        }
        const KNpc* l = entities_.find(m.launcher);
        if (l == nullptr) return false;
        out = l->pos();
        return true;
    }
    default: return true;
    }
}

bool KSubWorld::missle_check_beyond_region(KMissle& m, int dx, int dy)
{
    // 0x08074F10: the move by (dx, dy) in 1/1024 units.  A RelativePosType missile adds them to
    // its way from the anchor and stands at anchor + way; any other moves its offset, at most a
    // cell at a time, and crosses into the next cell (the next region: a step off the map fails).
    if (!m.on_map) return false;
    if (m.relative_pos_type != 0) {
        m.rel_x += dx;
        m.rel_y += dy;
        Pos base;
        if (!missle_relative_base(m, base)) return false;
        return missle_set_pos(m, Pos{base.x + static_cast<int>(m.rel_x >> 10), base.y + static_cast<int>(m.rel_y >> 10)});
    }
    if (dy == 0 && dx == 0) return true;
    if (std::abs(dx) > kMissleCellUnits || std::abs(dy) > kMissleCellUnits) return false;
    int ox = dx + m.x_offset;
    int oy = dy + m.y_offset;
    int cx = m.map_x;
    int cy = m.map_y;
    if (ox < 0) {
        ox += kMissleCellUnits;
        cx -= 1;
    } else if (ox > kMissleCellUnits) {
        ox -= kMissleCellUnits;
        cx += 1;
    }
    if (oy < 0) {
        oy += kMissleCellUnits;
        cy -= 1;
    } else if (oy > kMissleCellUnits) {
        oy -= kMissleCellUnits;
        cy += 1;
    }
    if (cx < 0 || cx >= missle_cells_x() || cy < 0 || cy >= missle_cells_y()) return false;
    m.map_x = cx;
    m.map_y = cy;
    m.x_offset = ox;
    m.y_offset = oy;
    return true;
}

// ---- the collisions -------------------------------------------------------------------------------

bool KSubWorld::missle_relation_ok(const KNpc& npc, const KNpc& launcher, int flags) const
{
    // the filter of the npc walks 0x080F2280 / 0x080F2610 / 0x080E20B0: a partner is skipped with
    // flag 4, the launcher itself taken with flag 2, players only with flag 0x20, flag 0x40 skips
    // the 0x08079200 check (the missiles always set it), the rest is GetRelation & flags
    if (npc.kind == KNpcKind::drop) return false;
    if (npc.kind != KNpcKind::player && npc.npc_kind == kind_partner && (flags & 4) != 0) return false;
    if ((flags & 2) != 0 && npc.id == launcher.id) return true;
    if ((flags & 0x20) != 0 && npc.kind != KNpcKind::player) return false;
    return (relation(launcher, npc) & flags) != 0;
}

EntityId KSubWorld::npc_at_cell(int cx, int cy, const KNpc& launcher, int flags) const
{
    // 0x080E20B0: the first npc standing in exactly this cell that the flags accept
    EntityId found;
    const Pos centre{cx * kMissleCell + kMissleCell / 2, cy * kMissleCell + kMissleCell / 2};
    grid_.for_each_within(centre, kMissleCell, [&](EntityId id) {
        if (found.valid()) return;
        const KNpc* e = entities_.find(id);
        if (e == nullptr || npc_cell_x(*e) != cx || npc_cell_y(*e) != cy) return;
        if (!missle_relation_ok(*e, launcher, flags)) return;
        found = id;
    });
    return found;
}

EntityId KSubWorld::first_npc_in_square(const KMissle& m, const KNpc& launcher, int range) const
{
    // 0x080748A0 + 0x080F2610: the first npc less than `range` cells away on both axes
    EntityId found;
    const Pos centre = missle_pos(m);
    grid_.for_each_within(centre, (range + 1) * kMissleCell, [&](EntityId id) {
        if (found.valid()) return;
        const KNpc* e = entities_.find(id);
        if (e == nullptr) return;
        const int dx = npc_cell_x(*e) - m.map_x;
        const int dy = npc_cell_y(*e) - m.map_y;
        if (dx >= range || dx <= -range || dy >= range || dy <= -range) return;
        if (!missle_relation_ok(*e, launcher, m.relation | 0x40)) return;
        found = id;
    });
    return found;
}

EntityId KSubWorld::missle_exact_target(const KMissle& m, const KNpc& launcher) const
{
    // 0x080749A0: a npc of the neighbouring cells within one cell of the missile's exact spot on
    // both axes (the offsets compared, 32 x 1024 inclusive)
    EntityId found;
    const Pos centre = missle_pos(m);
    const std::int64_t mx = static_cast<std::int64_t>(m.map_x) * kMissleCellUnits + m.x_offset;
    const std::int64_t my = static_cast<std::int64_t>(m.map_y) * kMissleCellUnits + m.y_offset;
    grid_.for_each_within(centre, 3 * kMissleCell, [&](EntityId id) {
        if (found.valid()) return;
        const KNpc* e = entities_.find(id);
        if (e == nullptr) return;
        const int dcx = npc_cell_x(*e) - m.map_x;
        const int dcy = npc_cell_y(*e) - m.map_y;
        if (dcx >= 2 || dcx <= -2 || dcy >= 2 || dcy <= -2) return;
        if (!missle_relation_ok(*e, launcher, m.relation | 0x40)) return;
        if (std::abs(npc_x1024(*e) - mx) > kMissleCellUnits) return;
        if (std::abs(npc_y1024(*e) - my) > kMissleCellUnits) return;
        found = id;
    });
    return found;
}

int KSubWorld::missle_check_collision(KMissle& m)
{
    // CheckCollision 0x08075770: -1 on the ground, 0 nothing, 1 something was in reach (whether or
    // not the blows landed - only an interval that is not due says 0)
    if (m.map_z <= 0) return -1;
    if (m.map_z > kMissleMaxHitHeight) return 0;
    if (!m.on_map) return -1;
    const KNpc* launcher = entities_.find(m.launcher);
    if (launcher == nullptr) return 0;
    if (m.collide_range > 1) {
        if (!first_npc_in_square(m, *launcher, m.collide_range).valid()) return 0;
    } else {
        bool exact = false;
        if (m.must_be_hit) {   // 0x08075800: a MoveKind 7 missile in its arrival frames hits its spot
            const int f = m.current_life;
            if (f >= m.arrive_from && f <= m.arrive_to) {
                if (f == m.arrive_to) m.must_be_hit = false;
                exact = true;
            }
        }
        if (exact) {
            if (!missle_exact_target(m, *launcher).valid()) return 0;
        } else if (!npc_at_cell(m.map_x, m.map_y, *launcher, m.relation | 0x40).valid()) {
            return 0;
        }
    }
    const int r = m.damage_range == 1 ? missle_process_collision(m, 1) : missle_process_collision(m);
    return r != -1 ? 1 : 0;
}

int KSubWorld::missle_process_collision(KMissle& m, int range)
{
    // ProcessCollision 0x08075630: with a DmgInterval only when it is due (-1 otherwise, and the
    // next time noted); the blows on every npc within `range` cells of the missile's cell that the
    // relation accepts, counted; the walk stops when the missile vanished (its hits ran out)
    if (m.damage_interval != 0) {
        if (m.next_damage_tick >= tick_) return -1;
        m.next_damage_tick = tick_ + static_cast<std::uint64_t>(m.damage_interval);
    }
    KNpc* launcher = entities_.find(m.launcher);
    if (launcher == nullptr || range <= 0) return 0;
    m.last_map_x = m.map_x;
    m.last_map_y = m.map_y;
    std::vector<EntityId> around;
    const Pos centre = missle_pos(m);
    grid_.for_each_within(centre, (range + 1) * kMissleCell, [&](EntityId id) {
        const KNpc* e = entities_.find(id);
        if (e == nullptr) return;
        const int dx = npc_cell_x(*e) - m.map_x;
        const int dy = npc_cell_y(*e) - m.map_y;
        if (dx * dx + dy * dy > range * range) return;
        if (!missle_relation_ok(*e, *launcher, m.relation | 0x40)) return;
        around.push_back(id);
    });
    int hits = 0;
    for (const EntityId id : around) {
        KNpc* e = entities_.find(id);
        if (e == nullptr) continue;
        if (missle_process_damage(m, *e)) ++hits;
        if (m.status == missle_status_vanished) break;
    }
    return hits;
}

int KSubWorld::missle_process_collision(KMissle& m)
{
    // 0x08075710: nothing for a ClientSend missile, else the blows within DmgRange
    if (m.client_send != 0) return 0;
    return missle_process_collision(m, m.damage_range);
}

bool KSubWorld::missle_process_damage(KMissle& m, KNpc& target)
{
    // ProcessDamage 0x080753F0: the miss roll, then every node of the payload on the target - the
    // blow, and after one that landed its states and its immediate attributes; then the hit count
    if (m.miss_rate != 0) {
        if (m.miss_rate > random(100)) {
            log::trace("zone.fight", "missile missed", {log::kv("missle", m.index), log::kv("target", target.id), log::kv("percent", m.miss_rate)});
            return false;
        }
        log::trace("zone.fight", "missile hit roll", {log::kv("missle", m.index), log::kv("target", target.id), log::kv("percent", m.miss_rate)});
    }
    if (!m.attribs) return false;
    KNpc* launcher = entities_.find(m.launcher);
    if (launcher == nullptr) return false;
    const int before = target.cur.life;
    for (const KMissleMagicAttribsData& node : *m.attribs) {
        if (receive_damage(target, *launcher, m.series, m.is_melee, node.damage_attribs.data(), m.use_attack_rating, m.do_hurt, m.relation, node.skill_id) == 0) continue;
        if (node.state_count > 0) {
            set_state_skill_effect(target, launcher->id, node.skill_id, node.level, node.state_attribs.data(), node.state_count, node.state_attribs[0].value[1]);
        }
        if (node.immediate_count > 0) set_immediately_skill_effect(target, launcher->id, node.immediate_attribs.data(), node.immediate_count);
    }
    sync_life(target, before, launcher->id);
    if (m.rest_hit_count != 0) {   // 0x0807553C
        if (--m.rest_hit_count == 0) missle_do_vanish(m, true);
        log::trace("zone.fight", "missile hits left", {log::kv("missle", m.index), log::kv("target", target.id), log::kv("count", m.rest_hit_count)});
    }
    return true;
}

void KSubWorld::missle_do_collision(KMissle& m)
{
    // DoCollision 0x08075340: the collide event, then gone with ColVanish or flying on
    if (m.status == missle_status_vanished || m.status == 3) return;
    if (m.collide_event) {
        if (const KSkill* sk = skill_of(m.skill_id, m.level)) missle_event(*sk, 3, m);
    }
    if (m.collide_vanish) missle_do_vanish(m, true);
    else m.status = missle_status_fly;
}

void KSubWorld::missle_do_vanish(KMissle& m, bool event)
{
    // DoVanish 0x08075210: once; the vanished event when asked; the removal after the frame
    if (m.status == missle_status_vanished) return;
    if (event && m.vanished_event) {
        if (const KSkill* sk = skill_of(m.skill_id, m.level)) missle_event(*sk, 4, m);
    }
    m.status = missle_status_vanished;
}

void KSubWorld::missle_event(const KSkill& skill, int type, KMissle& m)
{
    // OnMissleEvent 0x080EE810: the event skill (Fly / Collid / VanishedSkillId) at EventSkillLevel
    // (-1 = the skill's level) cast at the missile's spot by the missile when ByMissle, else at the
    // launcher's spot by the launcher; only a style 0 skill can be an event (vtable + 0x10 = GetStyle)
    if (!m.used()) return;
    KNpc* launcher = entities_.find(m.launcher);
    if (launcher == nullptr) return;
    int level = skill.row.event_skill_level;
    if (level == -1) level = skill.level;
    if (level <= 0) return;
    int id = 0;
    switch (type) {
    case 2:
        if (!skill.row.fly_event || skill.row.fly_skill_id <= 0) return;
        id = skill.row.fly_skill_id;
        break;
    case 3:
        if (!skill.row.collide_event || skill.row.collide_skill_id <= 0) return;
        id = skill.row.collide_skill_id;
        break;
    case 4:
        if (!skill.row.vanished_event || skill.row.vanished_skill_id <= 0) return;
        id = skill.row.vanished_skill_id;
        break;
    default: return;
    }
    const Pos at = skill.row.by_missle ? missle_pos(m) : launcher->pos();
    if (id > 1999 || level > 63) return;
    const KSkill* sk = skills_ ? skills_->get(id, level) : nullptr;
    if (sk == nullptr || sk->row.style != skill_style_missles) return;
    KCastParams p;
    p.at_pos = true;
    p.pos = at;
    if (skill.row.by_missle) cast_missles(*sk, &m, *launcher, p);
    else cast_missles(*sk, nullptr, *launcher, p);
}

} // namespace jx::zone
