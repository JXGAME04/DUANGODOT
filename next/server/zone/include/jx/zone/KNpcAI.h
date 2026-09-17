// KNpcAI of the old core (KNpcAI.cpp, the #ifdef _SERVER part): the six "普通" modes of the
// AIMode column of npcs.txt.  Ported line by line; the zone calls activate() for every npc with
// an AIMode each tick, like KNpc::Activate -> NpcAI.Activate(m_Index) while m_ProcessAI is set.
//
//   1  普通主动类1  active: hunts every enemy in sight            AIParam[0] patrol, [1..4] skill 1..4, [5],[6] wait/patrol when far
//   2  普通主动类2  active + heals (skill 1) or flees on low life  [1] life %, [2] chance, [3] heal chance, [4..6] skill 2..4, [7],[8] far, [9] heals max
//   3  普通主动类3  active + attacks (skill 1) or flees on low life
//   4  普通被动类1  passive: only strikes back at whoever hurt it (m_nPeopleIdx)
//   5  普通被动类2  passive + heal / flee
//   6  普通被动类3  passive + skill 1 / flee
#pragma once

#include "jx/ids.hpp"

namespace jx::zone {

class KSubWorld;
struct KNpc;

// NPC_RELATION of GameDataDef.h (bit flags)
enum KNpcRelation : int { relation_none = 1, relation_self = 2, relation_ally = 4, relation_enemy = 8, relation_dialog = 16 };
// NPCCAMP of GameDataDef.h
enum KNpcCamp : int { camp_begin = 0, camp_justice, camp_evil, camp_balance, camp_free, camp_animal, camp_event, camp_num };
// NPCKIND of GameDataDef.h (the Kind column of npcs.txt)
enum KNpcOldKind : int { kind_normal = 0, kind_player, kind_partner, kind_dialoger, kind_bird, kind_mouse, kind_num };

// KNpcSet::GenOneRelation: the camp / kind rules behind KNpcSet::m_RelationTable.
int g_GenOneRelation(int kind1, int kind2, int camp1, int camp2) noexcept;

class KNpcAI {
public:
    static constexpr int kMaxAiParam = 11;        // MAX_AI_PARAM (KNpc.h)
    static constexpr int kMiniAttackRange = 32;   // MINI_ATTACK_RANGE (KNpcAI::FollowAttack)
    static constexpr int kOldCell = 32;           // m_nCellWidth / m_nCellHeight of the old maps (GetNearestNpc scans cells)

    // KNpcAI::Activate for one npc (KSubWorld::tick calls it before the npc's per-frame update).
    static void activate(KSubWorld& w, KNpc& e);

private:
    static void process_type01(KSubWorld& w, KNpc& e);
    static void process_type02(KSubWorld& w, KNpc& e);
    static void process_type03(KSubWorld& w, KNpc& e);
    static void process_type04(KSubWorld& w, KNpc& e);
    static void process_type05(KSubWorld& w, KNpc& e);
    static void process_type06(KSubWorld& w, KNpc& e);

    static KNpc* lock_enemy(KSubWorld& w, KNpc& e);        // modes 1-3: m_nPeopleIdx if still in sight, else GetNearestNpc
    static KNpc* attacker(KSubWorld& w, KNpc& e);          // modes 4-6: m_nPeopleIdx (set by ReceiveDamage) if still there
    static EntityId nearest_enemy(const KSubWorld& w, const KNpc& e);   // GetNearestNpc(relation_enemy)
    static bool keep_active_range(KSubWorld& w, KNpc& e);
    static bool in_eyeshot(const KNpc& e, const KNpc& other) noexcept;
    static void common_action(KSubWorld& w, KNpc& e);
    static void follow_attack(KSubWorld& w, KNpc& e, KNpc& enemy);
    static void keep_attack_range(KSubWorld& w, KNpc& e, const KNpc& enemy, int range);
    static void flee(KSubWorld& w, KNpc& e, const KNpc& enemy);
    static bool set_active_skill(KNpc& e, int slot);      // KNpc::SetActiveSkill
    // the "enemy within the biggest skill radius: pick a skill" chain; 1 = skill chosen (then FollowAttack),
    // 0 = wait, -1 = the slot is empty (CommonAction already issued)
    static int choose_skill(KSubWorld& w, KNpc& e, int first_slot, const int* percent, int count);
    // modes 2/5 (heal) and 3/6 (attack with skill 1): true when the low-life branch used this decision
    static bool low_life_action(KSubWorld& w, KNpc& e, KNpc& enemy, bool heal);
    // enemy beyond every skill radius: wait / patrol / close in; true when handled
    static bool beyond_skill_range(KSubWorld& w, KNpc& e, KNpc& enemy, int wait_percent, int patrol_percent);
};

} // namespace jx::zone
