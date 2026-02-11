#pragma once

template <typename T, int maxSize>
class Stack
{
   private:
    T data_[maxSize];
    int count_ = 0;

   public:
    Stack() : count_(0) {}

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
