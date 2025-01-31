#ifndef __NET_TSQUEUE_HPP__
#define __NET_TSQUEUE_HPP__

#include <deque>
#include <mutex>

namespace net
{

template <typename T>
class tsqueue
{
public:
    tsqueue() = default;
    ~tsqueue() { clear(); }

    [[nodiscard]] const T& front() const noexcept
    {
        std::scoped_lock lock(mutex_);
        return deq_queue_.front();
    }

    [[nodiscard]] const T& back() const noexcept
    {
        std::scoped_lock lock(mutex_);
        return deq_queue_.back();
    }

    void push_back(const T& item)
    {
        std::scoped_lock lock(mutex_);
        deq_queue_.push_back(item);
    }

    void push_front(const T& item)
    {
        std::scoped_lock lock(mutex_);
        deq_queue_.push_back(item);
    }
    
    [[nodiscard]] bool empty() const noexcept
    {
        std::scoped_lock lock(mutex_);
        return deq_queue_.empty();
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        std::scoped_lock lock(mutex_);
        return deq_queue_.size();
    }

    void clear()
    {
        std::scoped_lock lock(mutex_);
        deq_queue_.clear();
    }

    T pop_front()
    {
        std::scoped_lock lock(mutex_);
        T item = std::move(deq_queue_.front());
        deq_queue_.pop_front();
        return item;
    }

    T popp_back()
    {
        std::scoped_lock lock(mutex_);
        T item = std::move(deq_queue_.front());
        deq_queue_.pop_back();
        return item;
    }

private:
    std::mutex mutex_;
    std::deque<T> deq_queue_;

private:
    tsqueue(const tsqueue&) = delete;
    tsqueue& operator=(const tsqueue&) = delete;
};

} // namespace net

#endif // __NET_TSQUEUE_HPP__

