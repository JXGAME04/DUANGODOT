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
