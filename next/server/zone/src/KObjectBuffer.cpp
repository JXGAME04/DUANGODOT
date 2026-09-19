#include "jx/zone/KObjectBuffer.h"

#include <cstring>

namespace jx::zone {

KObjectBufferSet& g_ObjectBuffers()
{
    static KObjectBufferSet set;
    return set;
}

KObjectBufferSet::Buffer* KObjectBufferSet::find(std::uint32_t id)
{
    const auto it = buffers_.find(id);
    return it == buffers_.end() ? nullptr : &it->second;
}

const KObjectBufferSet::Buffer* KObjectBufferSet::find(std::uint32_t id) const
{
    const auto it = buffers_.find(id);
    return it == buffers_.end() ? nullptr : &it->second;
}

std::uint32_t KObjectBufferSet::create()
{
    const std::lock_guard<std::mutex> lock(mutex_);
    if (++next_ == 0) ++next_;   // 0x0813029C: the counter + 1; 0 says "none" to the scripts
    buffers_[next_] = Buffer{};
    return next_;
}

std::uint32_t KObjectBufferSet::create_from(const std::uint8_t* data, std::size_t size)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    if (++next_ == 0) ++next_;
    Buffer& b = buffers_[next_];
    if (data != nullptr && size > 0) b.data.assign(data, data + size);
    return next_;
}

bool KObjectBufferSet::release(std::uint32_t id)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    return buffers_.erase(id) > 0;
}

bool KObjectBufferSet::exists(std::uint32_t id) const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    return find(id) != nullptr;
}

bool KObjectBufferSet::is_empty(std::uint32_t id) const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    const Buffer* b = find(id);
    return b == nullptr || b->read >= b->data.size();   // 0x080FC327: length 0
}

bool KObjectBufferSet::clear(std::uint32_t id)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    Buffer* b = find(id);
    if (b == nullptr) return false;
    b->data.clear();
    b->read = 0;
    return true;
}

bool KObjectBufferSet::copy(std::uint32_t dst, std::uint32_t src)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    const Buffer* s = find(src);
    Buffer* d = find(dst);
    if (s == nullptr || d == nullptr) return false;
    const std::vector<std::uint8_t> bytes(s->data.begin() + static_cast<std::ptrdiff_t>(s->read), s->data.end());   // src may be dst
    d->data = bytes;   // 0x080FF9A4: emptied first
    d->read = 0;
    return true;
}

bool KObjectBufferSet::append(std::uint32_t dst, std::uint32_t src)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    const Buffer* s = find(src);
    Buffer* d = find(dst);
    if (s == nullptr || d == nullptr) return false;
    const std::vector<std::uint8_t> bytes(s->data.begin() + static_cast<std::ptrdiff_t>(s->read), s->data.end());
    d->data.insert(d->data.end(), bytes.begin(), bytes.end());
    return true;
}

bool KObjectBufferSet::push(std::uint32_t id, const void* bytes, std::size_t size)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    Buffer* b = find(id);
    if (b == nullptr) return false;
    const auto* p = static_cast<const std::uint8_t*>(bytes);
    b->data.insert(b->data.end(), p, p + size);
    return true;
}

bool KObjectBufferSet::pop(std::uint32_t id, void* bytes, std::size_t size)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    Buffer* b = find(id);
    if (b == nullptr || b->data.size() - b->read < size) return false;   // 0x08128DBA: not enough left
    std::memcpy(bytes, b->data.data() + b->read, size);
    b->read += size;
    if (b->read >= b->data.size()) {   // 0x0810041A: nothing left, the offset back to 0
        b->data.clear();
        b->read = 0;
    }
    return true;
}

bool KObjectBufferSet::push_string(std::uint32_t id, const std::string& s)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    Buffer* b = find(id);
    if (b == nullptr) return false;
    const auto n = static_cast<std::uint32_t>(s.size() + 1);   // 0x08130C48: strlen + 1
    std::uint8_t len[4];
    std::memcpy(len, &n, sizeof len);
    b->data.insert(b->data.end(), len, len + 4);   // 0x08130CE8: the dword first
    b->data.insert(b->data.end(), s.begin(), s.end());
    b->data.push_back(0);
    return true;
}

std::optional<std::string> KObjectBufferSet::pop_string(std::uint32_t id)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    Buffer* b = find(id);
    if (b == nullptr) return std::nullopt;
    const std::size_t left = b->data.size() - b->read;
    if (left < 4) return std::nullopt;
    std::uint32_t n = 0;
    std::memcpy(&n, b->data.data() + b->read, 4);
    if (n == 0 || left < static_cast<std::size_t>(n) + 4) return std::nullopt;   // 0x081003D6 / 0x081003DE
    const char* start = reinterpret_cast<const char*>(b->data.data() + b->read + 4);
    std::string s(start, start + n - 1);              // 0x081003E5: the last byte is forced to NUL
    s.resize(std::strlen(s.c_str()));                 // lua_pushstring stops at the first NUL
    b->read += static_cast<std::size_t>(n) + 4;
    if (b->read >= b->data.size()) {
        b->data.clear();
        b->read = 0;
    }
    return s;
}

std::vector<std::uint8_t> KObjectBufferSet::unread(std::uint32_t id) const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    const Buffer* b = find(id);
    if (b == nullptr) return {};
    return std::vector<std::uint8_t>(b->data.begin() + static_cast<std::ptrdiff_t>(b->read), b->data.end());
}

std::size_t KObjectBufferSet::count() const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    return buffers_.size();
}

} // namespace jx::zone
