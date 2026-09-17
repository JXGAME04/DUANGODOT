// jx::core::CommandQueue / EventQueue - how work crosses a thread boundary (MASTER SPEC 19, 30,
// 73, 74, 75).
//
// The rule of the architecture is "one mutable entity, one owner".  A thread that is not the
// owner never touches the state: it pushes a command into the owner's queue and the owner
// applies it during its own tick.  That is why the network thread may not call Player.Move(),
// and why there is no global world mutex.
//
// Implementation: many producers, one consumer, a small mutex held only while moving a pointer
// sized item into a vector.  The consumer swaps the whole vector out in O(1) and then works
// without holding anything.  SPEC 75: correctness and low contention first; lock free only
// where a profiler proves it is needed.
#pragma once

#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

namespace jx::core {

template <class T>
class SwapQueue {
public:
    SwapQueue() = default;
    explicit SwapQueue(std::size_t reserve)
    {
        in_.reserve(reserve);
        out_.reserve(reserve);
    }

    SwapQueue(const SwapQueue&) = delete;
    SwapQueue& operator=(const SwapQueue&) = delete;

    // Any thread.  Returns false when the queue is full (max == 0 means unbounded).
    bool push(T item, std::size_t max = 0)
    {
        std::lock_guard lock(mutex_);
        if (max != 0 && in_.size() >= max) {
            ++dropped_;
            return false;
        }
        in_.push_back(std::move(item));
        ++pushed_;
        return true;
    }

    template <class... Args>
    bool emplace(Args&&... args)
    {
        std::lock_guard lock(mutex_);
        in_.emplace_back(std::forward<Args>(args)...);
        ++pushed_;
        return true;
    }

    // Owner thread only: hands every queued item to fn, in push order.  Returns how many ran.
    // New pushes during the drain land in the next batch, so a command that queues a command
    // can never spin the loop for ever.
    template <class Fn>
    std::size_t drain(Fn&& fn)
    {
        {
            std::lock_guard lock(mutex_);
            out_.swap(in_);
            in_.clear();
        }
        for (auto& item : out_) fn(std::move(item));
        const std::size_t n = out_.size();
        out_.clear();
        return n;
    }

    // Owner thread only: takes everything out at once (when the caller wants the vector).
    std::vector<T> take()
    {
        std::vector<T> result;
        {
            std::lock_guard lock(mutex_);
            result.swap(in_);
            in_.clear();
        }
        return result;
    }

    [[nodiscard]] std::size_t size() const
    {
        std::lock_guard lock(mutex_);
        return in_.size();
    }
    [[nodiscard]] bool empty() const { return size() == 0; }
    [[nodiscard]] std::uint64_t pushed() const
    {
        std::lock_guard lock(mutex_);
        return pushed_;
    }
    // How many pushes were refused because the queue was full (SPEC 70: backpressure is visible).
    [[nodiscard]] std::uint64_t dropped() const
    {
        std::lock_guard lock(mutex_);
        return dropped_;
    }
    void clear()
    {
        std::lock_guard lock(mutex_);
        in_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::vector<T> in_;    // producers append here
    std::vector<T> out_;   // the consumer works from here (owner thread only)
    std::uint64_t pushed_ = 0;
    std::uint64_t dropped_ = 0;
};

// Work going *into* an owner (network -> world, world -> world, scheduler -> worker).
template <class T>
class CommandQueue : public SwapQueue<T> {
public:
    using SwapQueue<T>::SwapQueue;
};

// Facts coming *out* of an owner (world -> network, world -> persistence, world -> scheduler).
template <class T>
class EventQueue : public SwapQueue<T> {
public:
    using SwapQueue<T>::SwapQueue;
};

} // namespace jx::core
