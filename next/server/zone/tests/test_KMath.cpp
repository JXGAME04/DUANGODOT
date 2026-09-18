#include <catch2/catch_test_macros.hpp>

#include "jx/zone/KMath.h"

using jx::zone::g_Dir64ToSprite;
using jx::zone::g_GetDirIndex;

TEST_CASE("64-direction index follows g_GetDirIndex of the JX2 server", "[math]")
{
    // jx_linux_y 0x080EEEC0: the nearest cell of g_nSin, mirrored as 64 - i for x2 >= x1
    CHECK(g_GetDirIndex(0, 0, 0, 100) == 0);       // down
    CHECK(g_GetDirIndex(0, 0, -100, 0) == 16);     // left
    CHECK(g_GetDirIndex(0, 0, 0, -100) == 32);     // up
    CHECK(g_GetDirIndex(0, 0, 100, 0) == 48);      // right
    CHECK(g_GetDirIndex(0, 0, -100, 100) == 8);    // down-left
    CHECK(g_GetDirIndex(0, 0, 100, -100) == 40);   // up-right
    CHECK(g_GetDirIndex(0, 0, -3, 100) == 0);
    CHECK(g_GetDirIndex(0, 0, 3, 100) == 0);       // 0 is never mirrored
    CHECK(g_GetDirIndex(0, 0, 20, 100) == 63);     // nsin 1013: 1019 is nearer than 1004 -> 1, mirrored
    CHECK(g_GetDirIndex(0, 0, 50, 200) == 62);     // nsin 994 between 1004 and 979, nearer 1004 -> 2
    CHECK(g_GetDirIndex(5, 5, 5, 5) == -1);
    // scene positions of Phượng Tường are five digits: no overflow in the distance
    CHECK(g_GetDirIndex(51758, 102105, 51830, 102161) == g_GetDirIndex(0, 0, 72, 56));
    // the tables are the binary's (truncated), not the rounded ones of the old client
    CHECK(jx::zone::kSin64[3] == 979);
    CHECK(jx::zone::kCos64[48] == 1024);
    CHECK(jx::zone::g_DirSin(64) == -1);
    CHECK(jx::zone::g_DirCos(-1) == -1);
}

TEST_CASE("64 directions map onto sprite directions", "[math]")
{
    CHECK(g_Dir64ToSprite(0, 8) == 0);
    CHECK(g_Dir64ToSprite(16, 8) == 2);
    CHECK(g_Dir64ToSprite(31, 8) == 4);
    CHECK(g_Dir64ToSprite(47, 8) == 6);
    CHECK(g_Dir64ToSprite(63, 8) == 0);
    CHECK(g_Dir64ToSprite(20, 1) == 0);
}
