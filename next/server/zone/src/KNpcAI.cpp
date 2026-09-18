// KNpcAI.cpp of the old core, server part: ProcessAIType01..06 and their helpers.  Every
// SendCommand(do_walk / do_skill / do_stand) of the old code is a KSubWorld::walk_to / cast_skill /
// do_stand here (ProcCommand ran the command in the same frame).  Differences from the old server
// are commented where they are.
#include "jx/zone/KNpcAI.h"

#include <cstdlib>
#include <tuple>

#include "jx/log.hpp"
#include "jx/zone/KMath.h"
#include "jx/zone/KNpc.h"
#include "jx/zone/KSubWorld.h"

namespace jx::zone {

int g_GenOneRelation(int kind1, int kind2, int camp1, int camp2) noexcept
{
    // 路人NPC没有战斗关系
    if (kind1 == kind_dialoger || kind2 == kind_dialoger) return relation_dialog;
    // 路人阵营没有战斗关系
    if (camp1 == camp_event || camp2 == camp_event) return relation_none;
    // 新手和动物还是战斗关系
    if ((camp1 == camp_begin && camp2 == camp_animal) || (camp1 == camp_animal && camp2 == camp_begin)) return relation_enemy;
    // 只要有一个新手，就不存在战斗关系(是同盟关系，大家帮新手)
    if (camp1 == camp_begin || camp2 == camp_begin) return relation_ally;
    // 两个都是玩家: 只要有一个玩家开了PK，就一定存在战斗关系
    if (kind1 == kind_player && kind2 == kind_player && (camp1 == camp_free || camp2 == camp_free)) return relation_enemy;
    // 同阵营为伙伴关系
    if (camp1 == camp2) return relation_ally;
    // 其他情况为战斗关系
    return relation_enemy;
}

namespace {

// KNpcSet::GetDistance / GetDistanceSquare in scene units
int distance(const KNpc& a, const KNpc& b) noexcept
{
    return g_GetDistance(a.pos().x, a.pos().y, b.pos().x, b.pos().y);
}

std::int64_t distance_square(const KNpc& a, const KNpc& b) noexcept
{
    const std::int64_t dx = a.pos().x - b.pos().x;
    const std::int64_t dy = a.pos().y - b.pos().y;
    return dx * dx + dy * dy;
}

} // namespace

void KNpcAI::activate(KSubWorld& w, KNpc& e)
{
    if (e.kind == KNpcKind::player) return;   // ProcessPlayer() is empty on the server
    if (e.life_max() == 0) return;
    if (e.next_ai_time > w.tick_) return;
    e.next_ai_time = w.tick_ + e.ai_max_time;
    switch (e.ai_mode) {
    case 1: process_type01(w, e); break;
    case 2: process_type02(w, e); break;
    case 3: process_type03(w, e); break;
    case 4: process_type04(w, e); break;
    case 5: process_type05(w, e); break;
    case 6: process_type06(w, e); break;
    default: break;
    }
}

// ---- helpers ---------------------------------------------------------------------------------

bool KNpcAI::in_eyeshot(const KNpc& e, const KNpc& other) noexcept
{
    return e.cur.vision_radius > distance(e, other);
}

// GetNearestNpc(relation_enemy): the old code scans the 32-unit cells of the vision radius in a
// fixed order (columns i = 0.., rows j = 0.., the four mirrored cells (+i,+j) (-i,+j) (-i,-j)
// (+i,-j)) and returns the first enemy it meets; the same order is reproduced with a sort key.
EntityId KNpcAI::nearest_enemy(const KSubWorld& w, const KNpc& e)
{
    const int range = e.cur.vision_radius / kOldCell;
    if (range <= 0) return EntityId{};
    const int cx = e.pos().x / kOldCell;   // m_MapX / m_MapY as absolute cells
    const int cy = e.pos().y / kOldCell;
    EntityId best;
    std::tuple<int, int, int, std::uint64_t> best_key;
    w.grid_.for_each_within(e.pos(), e.cur.vision_radius, [&](EntityId id) {
        if (id == e.id) return;
        const KNpc* found = w.entities_.find(id);
        if (found == nullptr) return;
        const KNpc& o = *found;
        if (!o.alive()) return;
        const int di = o.pos().x / kOldCell - cx;
        const int dj = o.pos().y / kOldCell - cy;
        const int i = std::abs(di);
        const int j = std::abs(dj);
        if (i >= range || j >= range || i * i + j * j > range * range) return;   // 去掉边角几个格子，保证视野是椭圆形
        if ((w.relation(e, o) & relation_enemy) == 0) return;
        int q = 3;
        if (di >= 0 && dj >= 0) q = 0;
        else if (di <= 0 && dj >= 0) q = 1;
        else if (di <= 0 && dj <= 0) q = 2;
        const auto key = std::tuple{i, j, q, id.value};
        if (!best || key < best_key) {
            best = id;
            best_key = key;
        }
    });
    return best;
}

KNpc* KNpcAI::lock_enemy(KSubWorld& w, KNpc& e)
{
    // 如果原本没有锁定敌人或者这个敌人跑太远，重新锁定敌人
    KNpc* enemy = w.find_mutable(e.people_id);
    if (enemy == nullptr || !enemy->alive() || !w.grid_.contains(enemy->id) || !in_eyeshot(e, *enemy)) {
        const EntityId id = nearest_enemy(w, e);
        if (id && id != e.people_id) {
            log::debug("zone.ai", "enemy locked", {log::kv("entity", e.id), log::kv("enemy", id), log::kv("mode", e.ai_mode)});
        }
        e.people_id = id;
        enemy = w.find_mutable(id);
    }
    return enemy;
}

KNpc* KNpcAI::attacker(KSubWorld& w, KNpc& e)
{
    // the old code trusts m_nPeopleIdx blindly; a slot that died or left is dropped here
    KNpc* enemy = w.find_mutable(e.people_id);
    if (enemy == nullptr || !enemy->alive() || !w.grid_.contains(enemy->id)) {
        e.people_id = EntityId{};
        return nullptr;
    }
    return enemy;
}

// KNpcAI::KeepActiveRange: back to the origin when beyond the active radius; the radius shrinks
// to a half meanwhile so the npc does not shuttle at the edge.
bool KNpcAI::keep_active_range(KSubWorld& w, KNpc& e)
{
    const int range = g_GetDistance(e.home.x, e.home.y, e.pos().x, e.pos().y);
    if (e.base.active_radius < range) e.cur.active_radius = e.base.active_radius / 2;
    if (e.cur.active_radius < range) {
        w.walk_to(e, e.home);
        return true;
    }
    e.cur.active_radius = e.base.active_radius;
    return false;
}

// KNpcAI::CommonAction: dialogers stand still; others walk back to the origin, one time in five
// to a random spot within half the active radius of it.
void KNpcAI::common_action(KSubWorld& w, KNpc& e)
{
    if (e.npc_kind == kind_dialoger) {
        w.do_stand(e);
        return;
    }
    int ox = 0, oy = 0;
    if (!w.rand_percent(80)) {
        ox = w.random(e.cur.active_radius / 2);
        oy = w.random(e.cur.active_radius / 2);
        if (ox & 1) ox = -ox;
        if (oy & 1) oy = -oy;
    }
    w.walk_to(e, Pos{e.home.x + ox, e.home.y + oy});
}

// KNpcAI::FollowAttack: too close -> step back to MINI_ATTACK_RANGE; within the active skill's
// radius and in sight -> cast it; else walk to the enemy.
void KNpcAI::follow_attack(KSubWorld& w, KNpc& e, KNpc& enemy)
{
    if (!w.grid_.contains(enemy.id)) return;   // m_RegionIndex < 0
    const int dist = distance(e, enemy);
    if (dist <= kMiniAttackRange) {
        keep_attack_range(w, e, enemy, kMiniAttackRange);
        return;
    }
    if (dist <= e.cur.attack_radius && in_eyeshot(e, enemy)) {
        w.cast_skill(e, enemy);
        return;
    }
    w.walk_to(e, enemy.pos());
}

// KNpcAI::KeepAttackRange: the spot `range` units before the enemy on the line from us to it.
void KNpcAI::keep_attack_range(KSubWorld& w, KNpc& e, const KNpc& enemy, int range)
{
    const Pos p1 = e.pos();
    const Pos p2 = enemy.pos();
    const int dir = g_GetDirIndex(p1.x, p1.y, p2.x, p2.y);
    // g_DirCos / g_DirSin return -1 for the "same spot" direction, which the old code shifted as well
    const Pos want{p2.x - ((range * g_DirCos(dir)) >> 10), p2.y - ((range * g_DirSin(dir)) >> 10)};
    w.walk_to(e, want);
}

// KNpcAI::Flee: away from the enemy, as far again as it is from us.
void KNpcAI::flee(KSubWorld& w, KNpc& e, const KNpc& enemy)
{
    const Pos p1 = e.pos();
    const Pos p2 = enemy.pos();
    log::debug("zone.ai", "flee", {log::kv("entity", e.id), log::kv("from", enemy.id), log::kv("life", e.life())});
    w.walk_to(e, Pos{p1.x * 2 - p2.x, p1.y * 2 - p2.y});
}

// KNpc::SetActiveSkill(nSkillIdx): the slot must hold a skill with a level.
bool KNpcAI::set_active_skill(KNpc& e, int slot)
{
    if (slot <= 0 || slot >= 5) return false;
    const KNpcSkillSlot& s = e.skills[slot];
    if (s.id == 0 || s.level <= 0) return false;
    // 0x08086D90: the cell must hold the skill with a current level (a level 0 cell is a skill unlearned)
    if (const KNpcSkill* c = e.skill_list.cell(slot); c == nullptr || c->id == 0 || c->current_level == 0) return false;
    e.active_skill_id = s.id;
    if (s.known) {   // g_SkillManager.GetSkill != NULL
        e.cur.attack_radius = s.attack_radius;
        e.active_skill_melee = s.melee;
        e.active_skill_self = s.target_self;
    }
    return true;
}

int KNpcAI::choose_skill(KSubWorld& w, KNpc& e, int first_slot, const int* percent, int count)
{
    const int r = w.random(100);
    int acc = 0;
    for (int i = 0; i < count; ++i) {
        acc += percent[i];
        if (r < acc) {
            if (!set_active_skill(e, first_slot + i)) {
                common_action(w, e);
                return -1;
            }
            return 1;
        }
    }
    return 0;   // 待机
}

bool KNpcAI::low_life_action(KSubWorld& w, KNpc& e, KNpc& enemy, bool heal)
{
    const int* p = e.ai_param;
    // 检测剩余生命是否符合条件，生命太少一定概率使用补血/攻击技能或逃跑
    if (e.life_max() == 0 || static_cast<std::int64_t>(e.life()) * 100 / e.life_max() >= p[1]) return false;
    if (!w.rand_percent(p[2])) return false;
    if (heal) {
        if (e.ai_add_life_time < p[9] && w.rand_percent(p[3])) {   // 使用补血技能
            set_active_skill(e, 1);
            w.cast_skill(e, e);
            ++e.ai_add_life_time;
            return true;
        }
    } else if (w.rand_percent(p[3])) {   // 使用攻击技能
        set_active_skill(e, 1);
        follow_attack(w, e, enemy);
        return true;
    }
    flee(w, e, enemy);   // 逃跑
    return true;
}

bool KNpcAI::beyond_skill_range(KSubWorld& w, KNpc& e, KNpc& enemy, int wait_percent, int patrol_percent)
{
    // 如果敌人在所有技能攻击范围之外，一定概率选择待机/巡逻/向敌人靠近
    if (distance_square(e, enemy) <= e.ai_param[kMaxAiParam - 1]) return false;
    const int r = w.random(100);
    if (r < wait_percent) return true;   // 待机
    if (r < wait_percent + patrol_percent) {   // 巡逻
        common_action(w, e);
        return true;
    }
    follow_attack(w, e, enemy);   // 向敌人靠近
    return true;
}

// ---- the six modes ---------------------------------------------------------------------------

//	普通主动类1: m_AiParam[0] 无敌人时候的巡逻概率; [1..4] 四种技能的使用概率; [5],[6] 看见敌人但比较远时，待机、巡逻的概率
void KNpcAI::process_type01(KSubWorld& w, KNpc& e)
{
    const int* p = e.ai_param;
    if (keep_active_range(w, e)) return;
    KNpc* enemy = lock_enemy(w, e);
    if (enemy == nullptr) {   // 周围没有敌人，一定概率待机/巡逻
        if (p[0] > 0 && w.rand_percent(p[0])) common_action(w, e);
        return;
    }
    if (beyond_skill_range(w, e, *enemy, p[5], p[6])) return;
    const int percent[4] = {p[1], p[2], p[3], p[4]};
    if (choose_skill(w, e, 1, percent, 4) != 1) return;
    follow_attack(w, e, *enemy);
}

//	普通主动类2: [0] 巡逻; [1] 生命百分比; [2] 处理概率; [3] 回复技能(技能1)概率; [4..6] 技能2 3 4; [7],[8] 远时待机、巡逻; [9] 回复次数上限
void KNpcAI::process_type02(KSubWorld& w, KNpc& e)
{
    const int* p = e.ai_param;
    if (keep_active_range(w, e)) return;
    KNpc* enemy = lock_enemy(w, e);
    if (enemy == nullptr) {
        if (p[0] > 0 && w.rand_percent(p[0])) common_action(w, e);
        return;
    }
    if (low_life_action(w, e, *enemy, true)) return;
    if (beyond_skill_range(w, e, *enemy, p[7], p[8])) return;
    const int percent[3] = {p[4], p[5], p[6]};
    if (choose_skill(w, e, 2, percent, 3) != 1) return;
    follow_attack(w, e, *enemy);
}

//	普通主动类3: like 2, but skill 1 is an attack used on low life instead of a heal
void KNpcAI::process_type03(KSubWorld& w, KNpc& e)
{
    const int* p = e.ai_param;
    if (keep_active_range(w, e)) return;
    KNpc* enemy = lock_enemy(w, e);
    if (enemy == nullptr) {
        if (p[0] > 0 && w.rand_percent(p[0])) common_action(w, e);
        return;
    }
    if (low_life_action(w, e, *enemy, false)) return;
    if (beyond_skill_range(w, e, *enemy, p[7], p[8])) return;
    const int percent[3] = {p[4], p[5], p[6]};
    if (choose_skill(w, e, 2, percent, 3) != 1) return;
    follow_attack(w, e, *enemy);
}

//	普通被动类1: only the npc that hurt us (m_nPeopleIdx) is fought; [0] 巡逻; [1..4] 技能; [5],[6] 远时待机、巡逻
void KNpcAI::process_type04(KSubWorld& w, KNpc& e)
{
    const int* p = e.ai_param;
    KNpc* enemy = attacker(w, e);
    if (enemy == nullptr) {   // 是否受到攻击，否，一定概率选择待机/巡逻
        if (p[0] > 0 && w.rand_percent(p[0])) common_action(w, e);
        return;
    }
    if (keep_active_range(w, e)) return;
    if (beyond_skill_range(w, e, *enemy, p[5], p[6])) return;
    const int percent[4] = {p[1], p[2], p[3], p[4]};
    if (choose_skill(w, e, 1, percent, 4) != 1) return;
    follow_attack(w, e, *enemy);
}

//	普通被动类2: passive + heal (skill 1) / flee on low life; [4..6] 技能 2 3 4; [7],[8] 远时待机、巡逻
void KNpcAI::process_type05(KSubWorld& w, KNpc& e)
{
    const int* p = e.ai_param;
    KNpc* enemy = attacker(w, e);
    if (enemy == nullptr) {
        if (p[0] > 0 && w.rand_percent(p[0])) common_action(w, e);
        return;
    }
    if (keep_active_range(w, e)) return;
    if (low_life_action(w, e, *enemy, true)) return;
    if (beyond_skill_range(w, e, *enemy, p[7], p[8])) return;
    const int percent[3] = {p[4], p[5], p[6]};
    if (choose_skill(w, e, 2, percent, 3) != 1) return;
    follow_attack(w, e, *enemy);
}

//	普通被动类3: passive + skill 1 / flee on low life
void KNpcAI::process_type06(KSubWorld& w, KNpc& e)
{
    const int* p = e.ai_param;
    KNpc* enemy = attacker(w, e);
    if (enemy == nullptr) {
        if (p[0] > 0 && w.rand_percent(p[0])) common_action(w, e);
        return;
    }
    if (keep_active_range(w, e)) return;
    if (low_life_action(w, e, *enemy, false)) return;
    if (beyond_skill_range(w, e, *enemy, p[7], p[8])) return;
    const int percent[3] = {p[4], p[5], p[6]};
    if (choose_skill(w, e, 2, percent, 3) != 1) return;
    follow_attack(w, e, *enemy);
}

} // namespace jx::zone
