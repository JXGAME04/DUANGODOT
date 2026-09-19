#include "jx/zone/KTabFile.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <system_error>
#include <utility>

namespace jx::zone {

bool KTabFile::load(const std::string& os_path)
{
    if (os_path.empty()) return false;
    std::ifstream in(os_path, std::ios::binary);
    if (!in) return false;
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    parse(std::move(bytes));
    return true;
}

void KTabFile::parse(std::string bytes)
{
    rows_.clear();
    width_ = 0;
    height_ = 0;
    // CreateTabOffset: the lines, a CRLF or a lone LF / CR each; the last one counts without a line end, a trailing
    // line end adds no line
    std::vector<std::string_view> lines;
    std::size_t start = 0;
    const std::string_view all(bytes);
    while (start < all.size()) {
        std::size_t end = start;
        while (end < all.size() && all[end] != '\r' && all[end] != '\n') ++end;
        lines.push_back(all.substr(start, end - start));
        if (end >= all.size()) break;
        if (all[end] == '\r' && end + 1 < all.size() && all[end + 1] == '\n') end += 2;
        else end += 1;
        start = end;
    }
    if (lines.empty()) lines.emplace_back();
    // the width is the header's tabs + 1; every row gets that many cells, the missing ones empty, the extra ones dropped
    width_ = 1 + static_cast<int>(std::count(lines[0].begin(), lines[0].end(), '\t'));
    rows_.reserve(lines.size());
    for (const std::string_view line : lines) {
        std::vector<std::string> cells;
        cells.reserve(static_cast<std::size_t>(width_));
        std::size_t s = 0;
        while (static_cast<int>(cells.size()) < width_) {
            std::size_t t = line.find('\t', s);
            if (t == std::string_view::npos) {
                cells.emplace_back(line.substr(s));
                break;
            }
            cells.emplace_back(line.substr(s, t - s));
            s = t + 1;
        }
        while (static_cast<int>(cells.size()) < width_) cells.emplace_back();
        rows_.push_back(std::move(cells));
    }
    height_ = static_cast<int>(rows_.size());
}

bool KTabFile::get_string(int row, int col, std::string& out, std::size_t max_len) const
{
    if (row < 1 || row > height_ || col < 1 || col > width_) return false;   // GetValue: outside the table
    const std::string& cell = rows_[static_cast<std::size_t>(row - 1)][static_cast<std::size_t>(col - 1)];
    if (cell.empty()) return false;   // an empty cell is "not there" (dwLength == 0)
    out = cell.size() > max_len ? cell.substr(0, max_len) : cell;
    return true;
}

int KTabFile::find_column(std::string_view name) const
{
    if (height_ < 1) return -1;
    const auto& header = rows_[0];
    for (int i = 0; i < width_; ++i) {
        const std::string& cell = header[static_cast<std::size_t>(i)];
        if (cell.empty()) continue;
        // the 2003 FindColumn copies the name's length of the cell and compares that
        const std::string_view cut(cell.data(), std::min(cell.size(), name.size()));
        if (cut == name) return i + 1;
    }
    return -1;
}

int KTabFile::find_row(int col, std::string_view value) const
{
    if (col < 1 || col > width_) return -1;
    for (int r = 1; r <= height_; ++r) {
        const std::string& cell = rows_[static_cast<std::size_t>(r - 1)][static_cast<std::size_t>(col - 1)];
        if (cell.empty()) continue;
        const std::string_view cut(cell.data(), std::min<std::size_t>(cell.size(), 0x80));
        if (cut == value) return r;
    }
    return -1;
}

int KTabFile::find_row(std::string_view column_name, std::string_view value) const
{
    const int col = find_column(column_name);
    return col < 1 ? -1 : find_row(col, value);
}

bool KTabFile::set_string(int row, int col, std::string value)
{
    if (row < 1 || row > height_ || col < 1 || col > width_) return false;
    rows_[static_cast<std::size_t>(row - 1)][static_cast<std::size_t>(col - 1)] = std::move(value);
    return true;
}

// ---- the cache ------------------------------------------------------------------------------------

KTabFileCache::KTabFileCache(std::string roots)
{
    set_roots(std::move(roots));
}

void KTabFileCache::set_roots(std::string roots)
{
    roots_.clear();
    std::stringstream ss(roots);
    std::string r;
    while (std::getline(ss, r, ';')) {
        if (!r.empty()) roots_.push_back(r);
    }
}

std::string KTabFileCache::resolve(const std::string& game_path) const
{
    std::string rel = game_path;
    std::replace(rel.begin(), rel.end(), '\\', '/');
    while (!rel.empty() && rel.front() == '/') rel.erase(rel.begin());
    if (rel.empty()) return {};
    for (const std::string& root : roots_) {
        const std::filesystem::path p = std::filesystem::path(root) / rel;
        std::error_code ec;
        if (std::filesystem::is_regular_file(p, ec)) return p.string();
        // the old server ran on a case-insensitive disk: try the folders and the name in lower case
        std::string lower = rel;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        const std::filesystem::path q = std::filesystem::path(root) / lower;
        if (std::filesystem::is_regular_file(q, ec)) return q.string();
    }
    return {};
}

int KTabFileCache::load(const std::string& file, const std::string& key, bool writable)
{
    if (file.empty() || key.empty()) return 0;   // 0x0814D1BD / 0x0814D1C5
    const auto it = tables_.find(key);
    if (it != tables_.end()) return it->second.path == file ? 1 : 0;   // 0x0814D2C8: the same table stays, another path is refused
    const std::string os_path = resolve(file);
    if (os_path.empty()) return 0;
    auto table = std::make_unique<KTabFile>();
    if (!table->load(os_path)) return 0;
    Entry e;
    e.path = file;
    e.writable = writable;
    e.table = std::move(table);
    tables_.emplace(key, std::move(e));
    return 1;
}

bool KTabFileCache::unload(const std::string& key)
{
    return tables_.erase(key) != 0;
}

KTabFile* KTabFileCache::find(const std::string& key)
{
    const auto it = tables_.find(key);
    return it == tables_.end() ? nullptr : it->second.table.get();
}

const KTabFile* KTabFileCache::find(const std::string& key) const
{
    const auto it = tables_.find(key);
    return it == tables_.end() ? nullptr : it->second.table.get();
}

KTabFileCache& g_TabFiles() noexcept
{
    static KTabFileCache cache;
    return cache;
}

std::mutex& g_TabFilesLock() noexcept
{
    static std::mutex lock;
    return lock;
}

}   // namespace jx::zone
