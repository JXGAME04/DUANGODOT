// KObjectBufferSet - the byte buffers the scripts pass around by id: OB_Create 0x08130230 .. OB_PopInt 0x08128D30 of jx_linux_y
// work on the set at 0x9780ce0 (a tree of id -> object 0x9780ce4, the id counter 0x9780cf8, a pool of 0x10-byte objects
// {data, capacity, read offset, length} with 0x1000-byte blocks grown by pages - 0x08057B60 creates, 0x08057D10 releases,
// 0x08057260 appends).  RemoteExecute 0x08100740 carries one to another server, whose receiver 0x08052800 builds one from the
// bytes and runs the function with it.  docs/LINUX-SERVER.md §35.
#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace jx::zone {

class KObjectBufferSet {
public:
    std::uint32_t create();                                                    // a fresh empty buffer -> its id (never 0)
    std::uint32_t create_from(const std::uint8_t* data, std::size_t size);     // one holding `size` bytes (the receiver 0x08052800)
    bool release(std::uint32_t id);                                            // gone; false when there was none
    [[nodiscard]] bool exists(std::uint32_t id) const;
    [[nodiscard]] bool is_empty(std::uint32_t id) const;   // OB_IsEmpty 0x080FC280: true unless the buffer exists and holds unread bytes
    bool clear(std::uint32_t id);                          // OB_Clear 0x080FC360: read offset and length 0
    bool copy(std::uint32_t dst, std::uint32_t src);       // OB_Copy 0x080FF870: dst emptied, then src's unread bytes
    bool append(std::uint32_t dst, std::uint32_t src);     // OB_Append 0x080FF710: src's unread bytes after dst's
    bool push(std::uint32_t id, const void* bytes, std::size_t size);   // OB_Push*: appended
    // OB_Pop*: the next `size` bytes, false when fewer remain (the script gets nil); a buffer read to its end starts over at 0
    bool pop(std::uint32_t id, void* bytes, std::size_t size);
    bool push_string(std::uint32_t id, const std::string& s);   // OB_PushString 0x08130B90: a dword (length + 1), the bytes, a NUL
    std::optional<std::string> pop_string(std::uint32_t id);    // OB_PopString 0x08100320: nullopt without a whole string (or length 0)
    [[nodiscard]] std::vector<std::uint8_t> unread(std::uint32_t id) const;   // what RemoteExecute sends (0x08100897)
    [[nodiscard]] std::size_t count() const;

private:
    struct Buffer {
        std::vector<std::uint8_t> data;   // the block: [read, data.size()) is unread (+8 the read offset, +0xc the length)
        std::size_t read = 0;
    };
    Buffer* find(std::uint32_t id);
    [[nodiscard]] const Buffer* find(std::uint32_t id) const;
    mutable std::mutex mutex_;
    std::unordered_map<std::uint32_t, Buffer> buffers_;
    std::uint32_t next_ = 0;
};

// the one set of the process (0x9780ce0)
KObjectBufferSet& g_ObjectBuffers();

} // namespace jx::zone
