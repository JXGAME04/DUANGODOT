// KMath.h of the old core: the 64-direction system (0 = down and clockwise on screen: 16 = left,
// 32 = up, 48 = right), its sine and cosine tables and the direction search - the way the JX2
// server has them (jx_linux_y, docs/LINUX-SERVER.md §13):
//
//   g_nSin  0x082E1080 (through the pointer 0x082E12EC)   1024 x cos(i x 5.625 deg), truncated
//   g_nCos  0x082E1180 (through 0x082E12F0)               -1024 x sin(i x 5.625 deg), truncated
//   g_GetSinCos(table, dir, 64)   0x08074160   table[dir], -1 outside 0..63
//   g_GetDirIndex                 0x080EEEC0   the direction of a vector (KnockBack 0x08087BC6 and
//                                              KMissle::OnFly 0x08075FDF inline the same search)
//
// The tables are read out of the binary verbatim: the missiles fly by them, so a cell that the
// old client's dongle tables rounded the other way (980 for 979) would move a missile a unit off.
// The Godot client keeps its own copy for drawing (client/scenes/KMath.gd).
#pragma once

#include <cmath>
#include <cstdint>

namespace jx::zone {

// g_nSin / g_nCos of the JX2 server, x1024: the unit vector of direction d on screen is
// (g_DirCos(d), g_DirSin(d)) = (-sin(d * 5.625 deg), cos(d * 5.625 deg)), so that 0 = down,
// 16 = left, 32 = up, 48 = right.
inline constexpr int kSin64[64] = {
    1024,  1019,  1004,   979,   946,   903,   851,   791,   724,   649,   568,   482,   391,   297,   199,   100,
       0,  -100,  -199,  -297,  -391,  -482,  -568,  -649,  -724,  -791,  -851,  -903,  -946,  -979, -1004, -1019,
   -1024, -1019, -1004,  -979,  -946,  -903,  -851,  -791,  -724,  -649,  -568,  -482,  -391,  -297,  -199,  -100,
       0,   100,   199,   297,   391,   482,   568,   649,   724,   791,   851,   903,   946,   979,  1004,  1019,
};
inline constexpr int kCos64[64] = {
       0,  -100,  -199,  -297,  -391,  -482,  -568,  -649,  -724,  -791,  -851,  -903,  -946,  -979, -1004, -1019,
   -1024, -1019, -1004,  -979,  -946,  -903,  -851,  -791,  -724,  -649,  -568,  -482,  -391,  -297,  -199,  -100,
       0,   100,   199,   297,   391,   482,   568,   649,   724,   791,   851,   903,   946,   979,  1004,  1019,
    1024,  1019,  1004,   979,   946,   903,   851,   791,   724,   649,   568,   482,   391,   297,   199,   100,
};

// The five elements (series 0 metal, 1 wood, 2 water, 3 fire, 4 earth) and how they feed and
// beat each other - g_nAccrueSeries / g_nConquerSeries of the old KMath, the tables jx_linux_y
// fills at 0x080741D0 (0x0830ED18: metal feeds water, water wood, wood fire, fire earth, earth
// metal; 0x0830ED2C: metal beats wood, wood earth, earth water, water fire, fire metal).
inline constexpr int kAccrueSeries[5] = {2, 3, 1, 4, 0};
inline constexpr int kConquerSeries[5] = {1, 4, 3, 0, 2};

// g_IsAccrue(src, des): src feeds des (jx_linux_y 0x08074190: table[src] == des, src within 0..4)
inline bool g_IsAccrue(int src, int des) noexcept { return src >= 0 && src < 5 && kAccrueSeries[src] == des; }
inline bool g_IsConquer(int src, int des) noexcept { return src >= 0 && src < 5 && kConquerSeries[src] == des; }

// g_GetSinCos(g_nSin / g_nCos, dir, 64) of jx_linux_y 0x08074160: the table cell of a direction,
// -1 for a direction outside 0..63 (the "same spot" direction -1 of g_GetDirIndex included, which
// the old code then shifted like any other value).
inline int g_DirSin(int dir) noexcept { return dir < 0 || dir >= 64 ? -1 : kSin64[dir]; }
inline int g_DirCos(int dir) noexcept { return dir < 0 || dir >= 64 ? -1 : kCos64[dir]; }

// g_GetDistance: integer Euclidean distance (truncated like the old (int)sqrt).
inline int g_GetDistance(std::int64_t x1, std::int64_t y1, std::int64_t x2, std::int64_t y2) noexcept
{
    return static_cast<int>(std::sqrt(static_cast<double>((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2))));
}

// g_GetDirIndex of the JX2 server (jx_linux_y 0x080EEEC0): direction 0..63 from (x1, y1) to
// (x2, y2) in scene units, -1 when they coincide.  nsin = (dy << 10) / distance is looked up in
// g_nSin: the last i (0..31) whose sine is still >= nsin, moved to i + 1 when that one is the
// nearer of the two, then mirrored for x2 >= x1 as 64 - i; 0 stays 0.  (The JX1 KMath.h searched
// a half-step boundary table and mirrored as 63 - i: a straight right was 47 there, it is 48 here.)
inline int g_GetDirIndex(std::int64_t x1, std::int64_t y1, std::int64_t x2, std::int64_t y2) noexcept
{
    const std::int64_t dx = x2 - x1;
    const std::int64_t dy = y2 - y1;
    if (dx == 0 && dy == 0) return -1;
    const auto dist = static_cast<std::int64_t>(std::sqrt(static_cast<double>(dx * dx + dy * dy)));
    if (dist == 0) return -1;
    const int nsin = static_cast<int>((dy * 1024) / dist);   // "<< 10", the division toward zero
    int k = 31;
    for (int i = 0; i < 32; ++i) {
        if (nsin > kSin64[i]) {
            k = i - 1;
            break;
        }
    }
    if (k < 0) k = 0;   // nsin above 1024 cannot happen; the binary would read the cell before the table
    if (kSin64[k] != nsin && kSin64[k] - nsin > nsin - kSin64[k + 1]) ++k;
    if (k == 0) return 0;
    if (dx < 0) return k;
    return 64 - k;
}

// The sprite direction (of `dirs`) a 64-direction value maps to (KSprControl::SetCurDir64).
inline int g_Dir64ToSprite(int dir64, int dirs) noexcept
{
    if (dirs <= 0) dirs = 1;
    int d = (dir64 + 32 / dirs) / (64 / dirs);
    if (d >= dirs) d -= dirs;
    return d;
}

} // namespace jx::zone
