#include "jx/process.hpp"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <sys/time.h>

#include <cstdio>
#include <unistd.h>
#endif

namespace jx {

ProcessUsage process_usage() noexcept
{
    ProcessUsage out;
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        out.rss_bytes = pmc.WorkingSetSize;
        out.peak_rss_bytes = pmc.PeakWorkingSetSize;
    }
    FILETIME created{}, exited{}, kernel{}, user{};
    if (GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) {
        const auto to_ms = [](const FILETIME& t) {
            const std::uint64_t ticks = (static_cast<std::uint64_t>(t.dwHighDateTime) << 32) | t.dwLowDateTime;
            return ticks / 10000;   // 100 ns units
        };
        out.cpu_ms = to_ms(kernel) + to_ms(user);
    }
#else
    // /proc/self/statm: size resident shared ... in pages
    if (std::FILE* f = std::fopen("/proc/self/statm", "r")) {
        unsigned long size = 0, resident = 0;
        if (std::fscanf(f, "%lu %lu", &size, &resident) == 2) {
            out.rss_bytes = static_cast<std::size_t>(resident) * static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
        }
        std::fclose(f);
    }
    rusage ru{};
    if (::getrusage(RUSAGE_SELF, &ru) == 0) {
        out.peak_rss_bytes = static_cast<std::size_t>(ru.ru_maxrss) * 1024;   // kilobytes on Linux
        out.cpu_ms = static_cast<std::uint64_t>(ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) * 1000 +
                     static_cast<std::uint64_t>(ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) / 1000;
    }
#endif
    return out;
}

} // namespace jx
