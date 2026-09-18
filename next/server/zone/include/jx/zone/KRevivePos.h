#pragma once
// \settings\revivepos.ini of the JX2 server (jxassets export-revive-pos -> revive_pos.json): one section
// per map with the revive / reference points KSubWorldSet 0x080F6D20 looks up (section "%u" = the map,
// key "%d" = the id -> "x,y" in absolute Mps, through KIniFile 0x0821F1F0) and the "region=a,b" ids the
// map owns.  KPlayer::LoadFrom 0x080C171D enters a character that still carries cUseRevive (a fresh one)
// at the point (irevivalid, irevivalx = KPlayer+0x10 / +0x14) and writes it to +0x18 / +0x1c; a point the
// table lacks sends it to map 57 at 50976,102208 (0x080C2038).  KPlayer::Revive(0) and the script
// SetRevPos(map, ref) (0x080B1E50) resolve the same table (docs/LINUX-SERVER.md §16.4).
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "jx/zone/KRegion.h"

namespace jx::zone {

struct KRevivePosTable {
    struct Map {
        int region_lo = 0;   // "region=a,b": the ids a..b of this map (0, 0 without the line)
        int region_hi = 0;
        std::unordered_map<int, Pos> points;   // id -> absolute Mps
    };
    std::unordered_map<std::uint32_t, Map> maps;
    static constexpr std::uint32_t kDefaultMap = 57;     // 0x080C2038: where an unknown point sends a character
    static constexpr Pos kDefaultPos{50976, 102208};     // 0xc720, 0x18f40

    static std::optional<KRevivePosTable> load(const std::string& file, std::string* error);
    // KSubWorldSet 0x080F6D20: the point `ref` of `map`, nothing when the ini has no such section or key
    [[nodiscard]] std::optional<Pos> point(std::uint32_t map, int ref) const noexcept;
    // the "region=a,b" line of the map
    [[nodiscard]] std::optional<std::pair<int, int>> region(std::uint32_t map) const noexcept;
    // tests and scripts: one point more
    void add(std::uint32_t map, int ref, Pos at);
};

} // namespace jx::zone
