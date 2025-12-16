#pragma once

#include <mutex>

template <typename T>
class ThreadSafeVariable
{
   private:
    T value_;
    mutable std::mutex mutex_;

   public:
    ThreadSafeVariable() noexcept = default;
    explicit ThreadSafeVariable(const T& initial) noexcept : value_(initial) {}

    T Get() const noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_;
    }

    void Set(const T& newValue) noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ = newValue;
    }
};
