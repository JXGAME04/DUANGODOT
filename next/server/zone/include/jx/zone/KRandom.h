// g_Random of the old engine (Engine/Src/KRandom.cpp, 2000) and of the JX2 server (jx_linux_y
// 0x08226AD0, the same two constants): a linear congruential generator,
//     seed = seed * 3877 + 29573;  return seed % n;
// The old core kept one global seed (g_GetRandomSeed / g_RandomSeed = 0x08226B00 / 0x08226AC0);
// an item remembers the seed it was rolled from (KItem+0x1e0 in the JX2 server) so that the same
// item can be rolled again.  Every roller of JX NEXT owns its KRandom instead - the simulation
// workers share nothing (MASTER SPEC 42) - and the sequence is the old one for the same seed.
#pragma once

#include <cstdint>

namespace jx::zone {

class KRandom {
public:
    explicit KRandom(std::uint32_t seed = 42) noexcept : seed_(seed) {}   // nRandomSeed = 42 of KRandom.cpp

    void seed(std::uint32_t s) noexcept { seed_ = s; }
    [[nodiscard]] std::uint32_t seed() const noexcept { return seed_; }

    // g_Random(nMax): 0 .. nMax - 1, and 0 for nMax == 0 (the seed does not move then either)
    std::uint32_t operator()(std::uint32_t n) noexcept
    {
        if (n == 0) return 0;
        seed_ = seed_ * 3877u + 29573u;
        return seed_ % n;
    }

    // g_Random as the old code calls it with an int; a range of zero or less gives 0
    int random(int n) noexcept { return n <= 0 ? 0 : static_cast<int>((*this)(static_cast<std::uint32_t>(n))); }

    // GetRandomNumber(nMin, nMax) of KCore.h: g_Random(nMax - nMin + 1) + nMin, both ends inside
    int between(int lo, int hi) noexcept { return random(hi - lo + 1) + lo; }

private:
    std::uint32_t seed_;
};

} // namespace jx::zone
