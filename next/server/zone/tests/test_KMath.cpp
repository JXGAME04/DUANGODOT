#include <catch2/catch_test_macros.hpp>

#include "jx/zone/KMath.h"

using jx::zone::g_Dir64ToSprite;
using jx::zone::g_GetDirIndex;

TEST_CASE("64-direction index follows the old g_GetDirIndex convention", "[math]")
{
    // the same vectors the client test checks (client/tests/run.gd)
    CHECK(g_GetDirIndex(0, 0, 0, 100) == 0);       // down
    CHECK(g_GetDirIndex(0, 0, -100, 0) == 16);     // left
    CHECK(g_GetDirIndex(0, 0, 0, -100) == 31);     // up with x equal
    CHECK(g_GetDirIndex(0, 0, 100, 0) == 47);      // right
    CHECK(g_GetDirIndex(0, 0, -100, 100) == 8);    // down-left
    CHECK(g_GetDirIndex(0, 0, 100, -100) == 39);   // up-right
    CHECK(g_GetDirIndex(0, 0, -3, 100) == 0);
    CHECK(g_GetDirIndex(0, 0, 3, 100) == 63);
    CHECK(g_GetDirIndex(5, 5, 5, 5) == -1);
    // scene positions of Phượng Tường are five digits: no overflow in the distance
    CHECK(g_GetDirIndex(51758, 102105, 51830, 102161) == g_GetDirIndex(0, 0, 72, 56));
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
