#pragma once

#include <array>

template <typename T, int maxSize>
class Stack
{
   private:
    std::array<T, maxSize> data_;
    int count_ = 0;

   public:
    Stack() = default;

    T Pop()
    {
        if (count_) return data_[--count_];
        return T{};
    }
    void Push(const T& value)
    {
        if (count_ < maxSize) data_[count_++] = value;
    }

    int Size() const { return count_; }
    bool Empty() const { return !count_; }

    T& Top() { return data_[count_ - 1]; }
};
