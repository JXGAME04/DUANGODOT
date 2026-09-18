#pragma once
// KFaction.h - the eleven factions (g_Faction of Core/Src/KFaction.h; \settings\faction\门派设定.ini read by
// KFactionSet::Init 0x08060C70 of jx_linux_y: sections "%d" 0..10 with Name / ShowName / Series (S_GOLD.. -> 0..4,
// default 0) / Camp (C_BEGIN.. -> 0..6, default 1 = C_JUSTICE); jxassets export-faction -> faction.json) and the
// faction record of a character (KPlayerFaction of Core/Src/KPlayerFaction.h; KPlayer+0x59cc: current, first added,
// last added, times joined - the ctor 0x080C2590 starts -1 / -1 / -1 / 0).
//   - 0x08060C00 FindByName(series, name): a series above 4 or an empty name -> -1, else the entry whose Name is
//     exactly `name` (strcmp), -1 when none.
//   - 0x08060BB0 Allows(series, index): series <= 4, index <= 10 and an entry with that index AND that series.
//   - 0x080C26F0 KPlayerFaction::Add(series, index): refused unless Allows; current = index, count += 1, first = index
//     when this is the first time, last = index.  0x080C25F0 (ClearFaction): current = -1 only.  0x080C25C0
//     (ClearFactionRecord): everything back to the ctor's.
//   - 0x080C2610 Camp(): current 0..10 -> that faction's camp (when it is not negative), current -1 -> 4 (C_FREE) when
//     the character joined once, 0 (C_BEGIN) when never.
//   - 0x080C2680 Name() / 0x080C27B0 LastName(): the Name of the current / last faction; -1 -> the G_FACTION_OLD string
//     when count != 0, "" when never (the string table's G_FACTION_OLD is the [Name] Old line of the ini here).
// docs/LINUX-SERVER.md §16.7.
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace jx::zone {

struct KFactionEntry {
    int index = 0;
    int series = 0;
    int camp = 1;
    std::string name;        // the code name the scripts pass to SetFaction ("shaolin")
    std::string show_name;   // UTF-8
};

class KFaction {
public:
    static constexpr int kCount = 11;            // 0x08060CF0: eleven entries of 0xcc bytes
    static constexpr int kSeriesCount = 5;
    static constexpr int kCampFree = 4;          // C_FREE: what a character that left is
    static constexpr int kCampBegin = 0;         // C_BEGIN: a character that never joined

    KFaction();
    static std::optional<KFaction> load(const std::string& file, std::string* error);

    // 0x08060C00
    [[nodiscard]] int id_by_name(int series, std::string_view name) const noexcept;
    // 0x08060BB0
    [[nodiscard]] bool allows(int series, int index) const noexcept;
    [[nodiscard]] const KFactionEntry* entry(int index) const noexcept;
    [[nodiscard]] const std::vector<int>* skills(int index) const noexcept;   // factionskill.txt (the scripts' list)
    [[nodiscard]] const std::string& old_name() const noexcept { return old_name_; }
    [[nodiscard]] const std::string& new_name() const noexcept { return new_name_; }

    // tests: one entry more
    void set(int index, int series, int camp, std::string name, std::string show_name = {});
    void set_names(std::string new_name, std::string old_name);
    void set_skills(int index, std::vector<int> ids);

private:
    std::vector<KFactionEntry> entries_;
    std::unordered_map<int, std::vector<int>> skills_;
    std::string new_name_;
    std::string old_name_;
};

// KPlayerFaction: the record at KPlayer+0x59cc
struct KPlayerFaction {
    int current = -1;   // m_nCurFaction   +0x59cc
    int first = -1;     // m_nFirstAddFaction +0x59d0 (never saved: LoadFrom 0x080C1A62 leaves it)
    int last = -1;      // m_nLastAddFaction  +0x59d4
    int count = 0;      // m_nAddTimes        +0x59d8

    // 0x080C26F0: false when the table refuses (series / index mismatch)
    bool add(const KFaction& table, int series, int index) noexcept;
    void clear_current() noexcept { current = -1; }                          // 0x080C25F0
    void reset() noexcept { current = first = last = -1; count = 0; }         // 0x080C25C0
    // 0x080C2610
    [[nodiscard]] int camp(const KFaction* table) const noexcept;
    // 0x080C2680 / 0x080C27B0
    [[nodiscard]] std::string name(const KFaction* table) const;
    [[nodiscard]] std::string last_name(const KFaction* table) const;
};

} // namespace jx::zone
