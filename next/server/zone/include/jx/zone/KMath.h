// KMath.h of the old client: the 64-direction index of a movement, 0 = down and clockwise on
// screen (16 = left, 32 = up, 48 = right).  The boundary table is 1024 * cos((i - 1/2) * 5.625
// degrees); the old table was read from the copy-protection dongle, so it is regenerated here
// and shared verbatim with the client (client/scenes/KMath.gd).
#pragma once

#include <cmath>
#include <cstdint>

namespace jx::zone {

inline constexpr int kDirSin[32] = {1024, 1022, 1012, 993, 964, 925, 878, 822, 758, 687, 609, 526, 437, 344, 248, 150,
                                    50,   -50,  -150, -248, -344, -437, -526, -609, -687, -758, -822, -878, -925, -964, -993, -1012};

// g_nSin / g_nCos of the old KMath (also dongle tables), x1024: the screen unit vector of direction d
// is (g_DirCos(d), g_DirSin(d)) = (-sin(d * 5.625 deg), cos(d * 5.625 deg)) so that 0 = down,
// 16 = left, 32 = up, 48 = right, matching g_GetDirIndex.  KNpc::ServeMove and KNpcAI::KeepAttackRange
// use them.
inline constexpr int kSin64[64] = {
    1024,  1019,  1004,   980,   946,   903,   851,   792,   724,   650,   569,   483,   392,   297,   200,   100,
       0,  -100,  -200,  -297,  -392,  -483,  -569,  -650,  -724,  -792,  -851,  -903,  -946,  -980, -1004, -1019,
   -1024, -1019, -1004,  -980,  -946,  -903,  -851,  -792,  -724,  -650,  -569,  -483,  -392,  -297,  -200,  -100,
       0,   100,   200,   297,   392,   483,   569,   650,   724,   792,   851,   903,   946,   980,  1004,  1019,
};
inline constexpr int kCos64[64] = {
       0,  -100,  -200,  -297,  -392,  -483,  -569,  -650,  -724,  -792,  -851,  -903,  -946,  -980, -1004, -1019,
   -1024, -1019, -1004,  -980,  -946,  -903,  -851,  -792,  -724,  -650,  -569,  -483,  -392,  -297,  -200,  -100,
       0,   100,   200,   297,   392,   483,   569,   650,   724,   792,   851,   903,   946,   980,  1004,  1019,
    1024,  1019,  1004,   980,   946,   903,   851,   792,   724,   650,   569,   483,   392,   297,   200,   100,
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

// g_DirSin / g_DirCos for the 64-direction system (the old ones return -1 for a bad direction).
inline int g_DirSin(int dir) noexcept { return dir < 0 || dir >= 64 ? -1 : kSin64[dir]; }
inline int g_DirCos(int dir) noexcept { return dir < 0 || dir >= 64 ? -1 : kCos64[dir]; }

// g_GetDistance: integer Euclidean distance (truncated like the old (int)sqrt).
inline int g_GetDistance(std::int64_t x1, std::int64_t y1, std::int64_t x2, std::int64_t y2) noexcept
{
    return static_cast<int>(std::sqrt(static_cast<double>((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2))));
}

// g_GetDirIndex: direction 0..63 from (x1, y1) to (x2, y2) in scene units, -1 when they coincide.
inline int g_GetDirIndex(std::int64_t x1, std::int64_t y1, std::int64_t x2, std::int64_t y2) noexcept
{
    if (x1 == x2 && y1 == y2) return -1;
    const auto dist = static_cast<std::int64_t>(std::sqrt(static_cast<double>((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2))));
    if (dist == 0) return -1;
    const std::int64_t nsin = ((y2 - y1) * 1024) / dist;   // "<< 10" in the old code, truncating toward zero
    int ret = -1;
    for (int i = 0; i < 32; ++i) {
        if (nsin > kDirSin[i]) break;
        ret = i;
    }
    if (x2 - x1 > 0) ret = 63 - ret;
    return ret;
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
