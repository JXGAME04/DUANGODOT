// What this process costs the machine, for the stats line.  MASTER SPEC 55 asks for measured
// numbers, and "how many players fit" is meaningless without the memory they took.
#pragma once

#include <cstddef>
#include <cstdint>

namespace jx {

struct ProcessUsage {
    std::size_t rss_bytes = 0;        // resident / working set now
    std::size_t peak_rss_bytes = 0;   // the highest it has been
    std::uint64_t cpu_ms = 0;         // user + kernel time this process has used
};

// Reads the numbers from the OS.  Never throws; missing values stay 0.
[[nodiscard]] ProcessUsage process_usage() noexcept;

} // namespace jx
