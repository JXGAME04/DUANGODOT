// KTabFile.h - the tab-separated tables of the old server as its scripts read them (Engine/Src/KTabFile.h of 2003:
// CreateTabOffset, GetString, FindRow, FindColumn) and the cache behind the TabFile_* library of the JX2 server
// (jx_linux_y: TabFile_Load 0x0814AEF0 -> 0x0814D1A0 on the read cache 0x978262c, the three-argument form 0x0814CDE0 on
// the writable one 0x9782644; TabFile_UnLoad 0x0814B040, TabFile_GetRowCount 0x0814A690 (GetHeight), TabFile_GetColCount
// 0x0814A5E0 (GetWidth), TabFile_GetCell 0x0814A740 (GetString(row, column or its name, "", buf, 0x400)),
// TabFile_Search 0x0814A960 (0x08227C90: the column by name, then 0x08227BE0 compares the cells (0x80 bytes) of the
// rows), TabFile_SetCell 0x0814A420, TabFile_Save 0x0814A3A0).  script/class/ktabfile.lua wraps it: getCell(col, row) is
// TabFile_GetCell(key, row + 1, col), getRow() is TabFile_GetRowCount(key) - 1.  docs/LINUX-SERVER.md §24.
//
// A table keeps the bytes of the file: the first line names the columns (its tabs + 1 = the width), every line after it
// is a row (an empty line too; a last line without a line end counts; a trailing line end adds none), a row's cells are
// its tab-separated parts up to the width, the missing ones empty.  Rows and columns count from 1 (row 1 is the header).
#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace jx::zone {

class KTabFile {
public:
    bool load(const std::string& os_path);            // KTabFile::Load: the whole file, then CreateTabOffset
    void parse(std::string bytes);                    // CreateTabOffset on these bytes
    [[nodiscard]] int width() const noexcept { return width_; }     // GetWidth
    [[nodiscard]] int height() const noexcept { return height_; }   // GetHeight
    // GetValue(row - 1, column - 1, out, size): false outside the table or when the cell is empty; at most `max_len` bytes
    bool get_string(int row, int col, std::string& out, std::size_t max_len = 0x400) const;
    // FindColumn: the first header cell that, cut to the name's length, equals the name (the 2003 quirk); 1-based, -1 none
    [[nodiscard]] int find_column(std::string_view name) const;
    // 0x08227BE0: the first row whose cell in `col` (cut to 0x80 bytes) equals the value; 1-based, -1 none
    [[nodiscard]] int find_row(int col, std::string_view value) const;
    [[nodiscard]] int find_row(std::string_view column_name, std::string_view value) const;   // 0x08227C90
    bool set_string(int row, int col, std::string value);   // SetValue: in this copy only

private:
    std::vector<std::vector<std::string>> rows_;
    int width_ = 0;
    int height_ = 0;
};

// The tables the scripts loaded, by the key they gave (TabFile_Load(file, key)): one cache for both the read and the
// writable form of the old server.  `roots` are the old server folders (";" apart) the game paths are resolved in:
// "\settings\x.txt", "/settings/x.txt" or "settings/x.txt" -> <root>/settings/x.txt, the first root that has it.
class KTabFileCache {
public:
    KTabFileCache() = default;
    explicit KTabFileCache(std::string roots);
    void set_roots(std::string roots);   // the folders again (the tables already loaded stay)
    // 0x0814D1A0: a key already loaded from the same path -> 1 (kept); loaded from another path -> 0; a file that cannot be
    // read -> 0; else the table under the key -> 1
    int load(const std::string& file, const std::string& key, bool writable);
    bool unload(const std::string& key);   // 0x0814B040
    [[nodiscard]] KTabFile* find(const std::string& key);
    [[nodiscard]] const KTabFile* find(const std::string& key) const;
    [[nodiscard]] std::string resolve(const std::string& game_path) const;   // the os path of a game path, "" when no root has it
    [[nodiscard]] std::size_t size() const noexcept { return tables_.size(); }
    [[nodiscard]] const std::vector<std::string>& roots() const noexcept { return roots_; }

private:
    struct Entry {
        std::string path;
        bool writable = false;
        std::unique_ptr<KTabFile> table;
    };
    std::vector<std::string> roots_;
    std::map<std::string, Entry> tables_;
};

// the cache of the process (0x978262c / 0x9782644 of jx_linux_y): main.cpp gives it the roots; a script's Include may
// call TabFile_Load before any world runs, so it does not hang off a world.  The simulation threads share it: the
// calls lock, a table's pointer is used within the call that found it
KTabFileCache& g_TabFiles() noexcept;
std::mutex& g_TabFilesLock() noexcept;

}   // namespace jx::zone
