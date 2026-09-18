// KSkillList.cpp - the JX2 KSkillList (jx_linux_y KNpc+0x248), each function after the binary's
// (docs/LINUX-SERVER.md §15).  The cell offsets in the comments are the ones of a cell (0x30
// bytes at KNpc+0x250 + 0x30 x cell).
#include "jx/zone/KSkillList.h"

#include <algorithm>
#include <cstdint>

#include "jx/zone/KSkill.h"

namespace jx::zone {

namespace {
constexpr int kMaxCellLevel = 63;   // a level / current level above 63 is treated as none
} // namespace

// ---- lookups ------------------------------------------------------------------------------------

int KSkillList::find_same(int id) const noexcept
{
    // 0x080E4290: cells 1..79 in order, the first whose id matches; 0 for id 0
    if (id == 0) return 0;
    for (int i = 1; i < kMaxNpcSkill; ++i) {
        if (at(i).id == id) return i;
    }
    return 0;
}

int KSkillList::find_free() const noexcept
{
    // 0x080E42E0
    for (int i = 1; i < kMaxNpcSkill; ++i) {
        if (at(i).id == 0) return i;
    }
    return 0;
}

int KSkillList::get_level(int id) const noexcept
{
    // 0x080E43F0
    const int i = find_same(id);
    if (i == 0) return 0;
    const int level = at(i).level;
    return level > kMaxCellLevel ? 0 : level;
}

int KSkillList::get_current_level(int id, bool with_inc) const noexcept
{
    // 0x080E4440
    const int i = find_same(id);
    if (i == 0) return 0;
    const int cur = at(i).current_level;
    if (cur > kMaxCellLevel) return 0;
    int sub = 0;
    if (!with_inc) {   // 0x080E4487: the nodes of this skill and of every skill (0)
        for (const KSkillLevelIncNode& n : inc_nodes) {
            if (n.skill_id == 0 || n.skill_id == id) sub += n.inc;
        }
    }
    return cur - sub;
}

int KSkillList::get_all_inc() const noexcept
{
    // 0x080E43B0: the first node whose skill is 0
    for (const KSkillLevelIncNode& n : inc_nodes) {
        if (n.skill_id == 0) return n.inc;
    }
    return 0;
}

int KSkillList::get_count() const noexcept
{
    // 0x080E4380
    int n = 0;
    for (int i = 1; i < kMaxNpcSkill; ++i) {
        if (at(i).id != 0) ++n;
    }
    return n;
}

int KSkillList::get_count(bool all, KSkillManager* mgr) const
{
    // 0x080E4B30: cells 0..79 with an id 1..2000 whose row instantiates; all = false leaves out
    // WeaponSkill rows, IsExpSkill rows and cells at level 0
    int n = 0;
    for (int i = 0; i < kMaxNpcSkill; ++i) {
        const KNpcSkill& c = at(i);
        if (c.id <= 0 || !valid_skill_id(c.id)) continue;
        const KSkill* sk = instance(mgr, c.id, 1);
        if (sk == nullptr) continue;
        if (!all) {
            if (sk->row.weapon_skill || sk->row.is_exp_skill || c.level == 0) continue;
        }
        ++n;
    }
    return n;
}

int KSkillList::get_total_level(KSkillManager* mgr) const
{
    // 0x080E4C00
    int sum = 0;
    for (int i = 0; i < kMaxNpcSkill; ++i) {
        const KNpcSkill& c = at(i);
        if (c.id <= 0 || !valid_skill_id(c.id)) continue;
        const KSkill* sk = instance(mgr, c.id, 1);
        if (sk == nullptr || sk->row.weapon_skill || sk->row.is_exp_skill) continue;
        sum += c.level;
    }
    return sum;
}

// ---- cool down / forbid -------------------------------------------------------------------------

bool KSkillList::can_cast(int id, std::uint64_t frame, int npc_level) const noexcept
{
    // 0x080E4540
    const int i = find_same(id);
    if (i == 0) return false;
    const KNpcSkill& c = at(i);
    if (c.current_level <= 0) return false;   // +0x18
    if (c.forbidden != 0) return false;       // +0x2c
    if (frame < c.next_cast_time) return false;   // +0x10 (unsigned compare)
    if (npc_level <= 0) return true;
    return npc_level >= c.req_level;   // +0x28
}

bool KSkillList::is_cooling(int id, std::uint64_t frame) const noexcept
{
    // 0x080E44D0
    const int i = find_same(id);
    return i != 0 && frame < at(i).next_cast_time;
}

void KSkillList::set_next_cast_time(int id, std::uint64_t frame, int cool_down) noexcept
{
    // 0x080E4640: +0x14 = the length, +0x10 = frame + the length
    const int i = find_same(id);
    if (i == 0) return;
    at(i).cool_down_time = cool_down;
    at(i).next_cast_time = frame + static_cast<std::uint64_t>(static_cast<std::int64_t>(cool_down));
}

std::uint64_t KSkillList::next_cast_time(int id) const noexcept
{
    const int i = find_same(id);   // 0x080E46A0
    return i == 0 ? 0 : at(i).next_cast_time;
}

int KSkillList::cool_down_time(int id) const noexcept
{
    const int i = find_same(id);   // 0x080E46F0
    return i == 0 ? 0 : at(i).cool_down_time;
}

void KSkillList::reduce_cool_time(int id, int frames) noexcept
{
    // 0x080E4740: the next cast time comes `frames` nearer when it is beyond them (unsigned), the
    // length shrinks when it is longer than them
    const int i = find_same(id);
    if (i == 0) return;
    KNpcSkill& c = at(i);
    const std::uint64_t f = static_cast<std::uint64_t>(static_cast<std::uint32_t>(frames));
    if (c.next_cast_time > f) c.next_cast_time -= f;
    if (static_cast<std::uint32_t>(frames) < static_cast<std::uint32_t>(c.cool_down_time)) c.cool_down_time -= frames;
}

void KSkillList::clear_cool_time(std::uint64_t frame) noexcept
{
    // 0x080E47B0: every cell held that is not only_inc and still cooling (next cast beyond the
    // frame) forgets its cool down
    for (int i = 1; i < kMaxNpcSkill; ++i) {
        KNpcSkill& c = at(i);
        if (c.only_inc || c.id == 0) continue;
        if (c.next_cast_time <= frame) continue;
        c.next_cast_time = 0;
        c.cool_down_time = 0;
    }
}

bool KSkillList::is_forbidden(int id) const noexcept
{
    const int i = find_same(id);   // 0x080E45C0
    return i != 0 && at(i).forbidden == 1;
}

void KSkillList::set_forbid_all(int v) noexcept
{
    // 0x080E4610
    forbid_all = v;
    for (int i = 1; i < kMaxNpcSkill; ++i) {
        if (at(i).id != 0) at(i).forbidden = v;
    }
}

void KSkillList::set_forbid(int id, int v) noexcept
{
    const int i = find_same(id);   // 0x080AE9E0: +0x2c of the cell
    if (i != 0) at(i).forbidden = v;
}

// ---- cells --------------------------------------------------------------------------------------

void KSkillList::set_npc_skill(int no, int id, int level) noexcept
{
    // 0x080E4310
    if (level <= 0 || no <= 0 || id <= 0 || no > 0x4f) return;
    KNpcSkill& c = at(no);
    c.id = id;
    c.current_level = level;
    c.level = level;
    c.max_level = level;
    c.max_times = 0;
    c.remain_times = 0;
}

KSkillLevelIncNode* KSkillList::inc_node(int skill_id) noexcept
{
    for (KSkillLevelIncNode& n : inc_nodes) {
        if (n.skill_id == skill_id) return &n;
    }
    return nullptr;
}

const KSkill* KSkillList::instance(KSkillManager* mgr, int id, int level)
{
    if (mgr == nullptr) return nullptr;
    return mgr->get(id, level);
}

void KSkillList::free_cell(int idx) noexcept
{
    // 0x080E597C: id, level, MaxTimes, RemainTimes, exp, current level and the only_inc flag go
    // (the cool down, max level, req level and forbid flag stay in the cell)
    KNpcSkill& c = at(idx);
    c.id = 0;
    c.level = 0;
    c.max_times = 0;
    c.remain_times = 0;
    c.exp = 0;
    c.current_level = 0;
    c.only_inc = false;
}

void KSkillList::update_enhance(int idx, int old_level, int new_level, KSkillManager* mgr)
{
    // 0x080E4CA0: the row must be one of style 0..3 / 14 (the TSkillInfo style); the instance of
    // the old level takes its addskilldamage values out of the map (an entry is made at 0 when
    // missing), the instance of the new level adds its own.  A level outside 1..63 has no instance.
    const KNpcSkill& c = at(idx);
    if (!valid_skill_id(c.id) || mgr == nullptr) return;
    const KSkillTable* table = mgr->table();
    const KSkillRow* info = table != nullptr ? table->info(c.id) : nullptr;
    if (info == nullptr || !enhance_style(info->style)) return;
    const KSkill* old_sk = old_level > 0 && old_level <= kMaxCellLevel ? instance(mgr, c.id, old_level) : nullptr;
    const KSkill* new_sk = new_level > 0 && new_level <= kMaxCellLevel ? instance(mgr, c.id, new_level) : nullptr;
    for (std::size_t i = 0; i < 6; ++i) {
        if (old_sk != nullptr) {
            const KSkill::AddSkillDamage& d = old_sk->add_skill_damage[i];
            if (d.skill_id > 0) enhance[d.skill_id] -= d.value;
        }
        if (new_sk != nullptr) {
            const KSkill::AddSkillDamage& d = new_sk->add_skill_damage[i];
            if (d.skill_id > 0) enhance[d.skill_id] += d.value;
        }
    }
}

int KSkillList::increase_level(int idx, int delta, KSkillListHost& host)
{
    // 0x080E5010
    if (idx < 1 || idx > 0x4f) return 0;
    if (delta == 0) return 1;
    KNpcSkill& c = at(idx);
    const int old_cur = c.current_level;   // [ebp-0x1c]
    int new_level;
    if (delta > 0) {
        if (c.level != 0) {
            new_level = c.level + delta;
        } else {   // 0x080E50B4: a skill learned from level 0 takes the "every skill" increments
            c.current_level = old_cur + get_all_inc();
            new_level = c.level + delta;
        }
    } else {
        new_level = c.level + delta;
        if (new_level == 0) {   // 0x080E5054: a skill unlearned gives the "every skill" increments back
            c.current_level = old_cur - get_all_inc();
            new_level = c.level + delta;
        }
    }
    // 0x080E50F2
    const int new_cur = delta + c.current_level;
    c.level = new_level;
    c.current_level = new_cur;
    c.exp = 0;
    const KSkill* sk;
    if (new_cur == 0) {
        if (!valid_skill_id(c.id)) return 0;
        sk = instance(host.skills, c.id, 1);
    } else {
        if (!valid_skill_id(c.id)) return 0;
        if (new_cur <= 0 || new_cur > kMaxCellLevel) return 0;
        sk = instance(host.skills, c.id, new_cur);
    }
    if (sk == nullptr) return 0;
    const int style = sk->row.style;
    if (enhance_style(style)) update_enhance(idx, old_cur, c.current_level, host.skills);   // 0x080E51A3
    if (style != skill_style_passivity_npc_state) return 1;
    // 0x080E51B9: a passive skill is cast again at its new level, or its state removed at 0
    if (c.current_level > 0) {
        if (host.npc_level < sk->row.req_level) return 1;
        if (host.cast_passive) host.cast_passive(*sk);
        return 1;
    }
    if (host.remove_state) host.remove_state(c.id);   // 0x080E5280
    return 1;
}

int KSkillList::change_current_level(int idx, int delta, KSkillListHost& host)
{
    // 0x080E56A0
    if (idx == 0) {
        for (int i = 1; i < kMaxNpcSkill; ++i) {
            KNpcSkill& c = at(i);
            if (c.id <= 0 || c.level <= 0) continue;
            const int old_cur = c.current_level;
            const int new_cur = std::min(delta + old_cur, kMaxCurrentLevel);
            c.current_level = new_cur;
            if (new_cur <= 0) {   // 0x080E56C0 -> 0x080E5A48
                if (!valid_skill_id(c.id)) continue;
                const KSkill* sk1 = instance(host.skills, c.id, 1);
                if (sk1 == nullptr) continue;
                if (sk1->row.style == skill_style_passivity_npc_state && host.remove_state) host.remove_state(c.id);
            } else {
                if (!valid_skill_id(c.id) || new_cur > kMaxCellLevel) continue;
                const KSkill* sk = instance(host.skills, c.id, new_cur);
                if (sk == nullptr) continue;
                if (sk->row.style == skill_style_passivity_npc_state && host.npc_level >= sk->row.req_level && host.cast_passive) {
                    host.cast_passive(*sk);
                }
            }
            // 0x080E574F: the enhance map when the current level moved
            if (c.current_level == old_cur) continue;
            if (!valid_skill_id(c.id)) return 0;
            const KSkill* sk1 = instance(host.skills, c.id, 1);
            if (sk1 == nullptr) return 0;
            if (enhance_style(sk1->row.style)) update_enhance(i, old_cur, c.current_level, host.skills);
        }
        return 1;
    }
    // 0x080E5858: one cell
    if (idx < 1 || idx > 0x4f) return 1;
    KNpcSkill& c = at(idx);
    const int old_cur = c.current_level;
    int new_cur = delta + old_cur;
    if (new_cur > kMaxCurrentLevel) new_cur = kMaxCurrentLevel;
    c.current_level = new_cur;
    if (new_cur > 0) {   // 0x080E59C3
        if (!valid_skill_id(c.id)) return 0;
        if (new_cur == kMaxCurrentLevel) return 0;   // the binary refuses exactly 64 here
        const KSkill* sk = instance(host.skills, c.id, new_cur);
        if (sk == nullptr) return 0;
        if (sk->row.style == skill_style_passivity_npc_state && host.npc_level >= sk->row.req_level && host.cast_passive) {
            host.cast_passive(*sk);
        }
    }
    // 0x080E588C
    if (old_cur != c.current_level) {
        if (!valid_skill_id(c.id)) return 0;
        const KSkill* sk1 = instance(host.skills, c.id, 1);
        if (sk1 == nullptr) return 0;
        if (enhance_style(sk1->row.style)) update_enhance(idx, old_cur, c.current_level, host.skills);
    }
    // 0x080E5910
    if (c.current_level > 0) return 1;
    if (!valid_skill_id(c.id)) return 0;
    const KSkill* sk1 = instance(host.skills, c.id, 1);
    if (sk1 == nullptr) return 0;
    if (sk1->row.style == skill_style_passivity_npc_state && host.remove_state) host.remove_state(c.id);
    if (c.only_inc) free_cell(idx);   // 0x080E596C: a cell that lived on increments only goes
    return 1;
}

int KSkillList::add_level_inc(int id, int delta, KSkillListHost& host)
{
    // 0x080E5BF0
    if (delta == 0) return 0;
    if (id < 0) id = -id;
    int idx = 0;
    if (id != 0) {
        idx = find_same(id);
        if (idx == 0) {   // 0x080E5CAC: a cell that lives on the increments only
            idx = find_free();
            if (idx == 0) return 0;
            KNpcSkill& c = at(idx);
            c.id = id;
            c.level = 0;
            c.current_level = 0;
            c.max_times = 0;
            c.remain_times = 0;
            c.only_inc = true;
            c.forbidden = forbid_all;
        }
    }
    // 0x080E5C11
    if (KSkillLevelIncNode* n = inc_node(id); n != nullptr) {
        n->inc += delta;
        change_current_level(idx, delta, host);
        if (n->inc == 0) {
            inc_nodes.erase(std::find_if(inc_nodes.begin(), inc_nodes.end(), [&](const KSkillLevelIncNode& x) { return &x == n; }));
        }
        return 1;
    }
    inc_nodes.push_back(KSkillLevelIncNode{id, delta});   // 0x080E5D10: a new node
    change_current_level(idx, delta, host);
    return 1;
}

int KSkillList::add(int id, int level, int exp, int max_level_override, int max_level_addon, KSkillListHost& host)
{
    // 0x080E5420
    if (level < 0 || id <= 0) return 0;
    int max_level = max_level_override;
    if (max_level <= 0) {   // 0x080E5538: the row's MaxLevel
        max_level = 0;
        if (valid_skill_id(id) && host.skills != nullptr && host.skills->table() != nullptr) {
            if (const KSkillRow* info = host.skills->table()->info(id); info != nullptr) max_level = info->max_level;
        }
    }
    const KSkill* sk = valid_skill_id(id) && level > 0 && level <= kMaxCellLevel ? instance(host.skills, id, level) : nullptr;
    max_level += max_level_addon;   // 0x080E5570: a reborn player's Player+0x8600
    int idx = find_same(id);
    if (idx != 0) {
        KNpcSkill& c = at(idx);
        const int delta = level - c.level;
        c.only_inc = false;
        c.max_times = 0;
        c.remain_times = 0;
        increase_level(idx, delta, host);
        c.exp = exp;
        c.max_level = max_level;
        c.req_level = sk != nullptr ? sk->row.req_level : 0;
        return idx;
    }
    idx = find_free();
    if (idx == 0) return 0;
    KNpcSkill& c = at(idx);
    c.id = id;
    c.level = 0;
    c.current_level = 0;
    c.only_inc = false;
    increase_level(idx, level, host);
    c.max_times = 0;
    c.exp = exp;
    c.remain_times = 0;
    c.max_level = max_level;
    c.forbidden = forbid_all;
    c.req_level = sk != nullptr ? sk->row.req_level : 0;
    return idx;
}

void KSkillList::remove(int id, KSkillListHost& host)
{
    // 0x080E52D0
    if (!valid_skill_id(id)) return;
    const int idx = find_same(id);
    if (idx == 0) return;
    KNpcSkill& c = at(idx);
    increase_level(idx, -c.level, host);
    c.exp = 0;
    if (c.current_level > 0) {
        c.only_inc = true;
    } else {
        c.id = 0;
    }
}

void KSkillList::clear_attrib(KSkillManager* mgr)
{
    // 0x0807F341: cells 1..79 held whose current level is above the learned level
    for (int i = 1; i < kMaxNpcSkill; ++i) {
        KNpcSkill& c = at(i);
        if (c.id == 0 || c.current_level <= c.level) continue;
        update_enhance(i, c.current_level, c.level, mgr);
        c.current_level = c.level;
    }
}

bool KSkillList::cast_passives_at_level(KSkillListHost& host)
{
    // 0x080E4A00: cells 1..79 with a current level 1..63, row 1..2000 (missing -> nothing at all)
    bool cast = false;
    for (int i = 1; i < kMaxNpcSkill; ++i) {
        const KNpcSkill& c = at(i);
        if (c.id <= 0 || c.current_level <= 0) continue;
        if (!valid_skill_id(c.id) || c.current_level > kMaxCellLevel) return false;
        const KSkill* sk = instance(host.skills, c.id, c.current_level);
        if (sk == nullptr) return false;
        if (sk->row.style != skill_style_passivity_npc_state) continue;
        if (host.npc_level != sk->row.req_level) continue;
        if (host.cast_passive) host.cast_passive(*sk);
        cast = true;
    }
    return cast;
}

int KSkillList::rollback(KSkillListHost& host)
{
    // 0x080E5370: cells 0..79; the WeaponSkill and IsExpSkill rows stay
    int sum = 0;
    for (int i = 0; i < kMaxNpcSkill; ++i) {
        const KNpcSkill& c = at(i);
        if (c.id <= 0 || !valid_skill_id(c.id)) continue;
        const KSkill* sk = instance(host.skills, c.id, 1);
        if (sk == nullptr || sk->row.weapon_skill || sk->row.is_exp_skill) continue;
        const int level = c.level;
        sum += level;
        increase_level(i, -level, host);
    }
    return sum;
}

// ---- experience ---------------------------------------------------------------------------------

int KSkillList::exp_percent(int idx, KSkillManager* mgr) const
{
    // 0x080E4F20
    if (idx < 1 || idx > 0x4f) return 0;
    const KNpcSkill& c = at(idx);
    if (c.exp <= 0) return 0;
    if (!valid_skill_id(c.id) || c.level <= 0 || c.level > kMaxCellLevel) return 0;
    const KSkill* sk = instance(mgr, c.id, c.level);
    if (sk == nullptr || !sk->row.is_exp_skill) return 0;
    const std::uint32_t need = static_cast<std::uint32_t>(sk->skill_exp);
    if (static_cast<std::uint32_t>(c.exp) >= need) return 1024;
    return static_cast<int>((static_cast<std::int64_t>(c.exp) << 10) / static_cast<std::int64_t>(static_cast<int>(need)));
}

KSkillList::ExpResult KSkillList::add_skill_exp(const KMagicAttrib& attrib, bool percent_mode, KSkillListHost& host)
{
    // 0x080E5D90 (the list's part)
    ExpResult r;
    const int id = attrib.value[0];
    if (!valid_skill_id(id)) return r;
    const int idx = find_same(id);
    if (idx == 0) return r;
    KNpcSkill& c = at(idx);
    if (c.level <= 0 || c.level > kMaxCellLevel) return r;
    const KSkill* sk = instance(host.skills, id, c.level);
    if (sk == nullptr || !sk->row.is_exp_skill) return r;
    r.handled = true;
    r.idx = idx;
    r.old_level = c.level;
    r.old_percent = exp_percent(idx, host.skills);
    const int need = sk->skill_exp;   // +0x11c
    int add;
    if (percent_mode) {   // 0x080E608A: need x value / 10000 in 64 bits
        add = static_cast<int>(static_cast<std::int64_t>(need) * static_cast<std::int64_t>(attrib.value[1]) / 10000);
    } else {
        add = attrib.value[1];
    }
    int max_level = 0;
    if (host.skills != nullptr && host.skills->table() != nullptr) {
        if (const KSkillRow* info = host.skills->table()->info(id); info != nullptr) max_level = info->max_level;
    }
    if (static_cast<std::uint32_t>(c.level) < static_cast<std::uint32_t>(max_level)) {   // 0x080E5F5A
        int exp = add + c.exp;
        bool reached;
        if (exp < 0) {   // 0x080E60BD: an overflow of a positive add reaches the need, a negative one empties
            if (add < 0) {
                c.exp = 0;
                reached = need <= 0;
            } else {
                c.exp = 0x7fffffff;
                reached = true;
            }
        } else {
            c.exp = exp;
            reached = need <= exp;
        }
        if (reached) {   // 0x080E60D2
            c.exp = need;
            r.level_reached = true;
            if ((attrib.value[2] & 1) == 0) {
                increase_level(idx, 1, host);
                r.level_up = true;
            }
        }
    }
    return r;
}

// ---- role data ----------------------------------------------------------------------------------

std::vector<KSkillSaved> KSkillList::serialize() const
{
    // 0x080E48D0: cells 0..79 holding a skill, the only_inc ones left out
    std::vector<KSkillSaved> out;
    for (int i = 0; i < kMaxNpcSkill; ++i) {
        const KNpcSkill& c = at(i);
        if (c.id <= 0 || c.only_inc) continue;
        out.push_back(KSkillSaved{static_cast<std::int16_t>(c.id), static_cast<std::int16_t>(c.level), c.exp});
    }
    return out;
}

void KSkillList::deserialize(const std::vector<KSkillSaved>& saved, int max_level_addon, KSkillListHost& host)
{
    for (const KSkillSaved& s : saved) add(s.id, s.level, s.exp, 0, max_level_addon, host);
}

} // namespace jx::zone
