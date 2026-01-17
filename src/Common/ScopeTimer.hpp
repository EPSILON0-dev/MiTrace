#pragma once

#include <chrono>
#include <iostream>
#include <string>

template <typename T = float>
class ScopeTimer
{
   private:
    T& targetVariable_;
    std::chrono::steady_clock::time_point startTime_;

   public:
    explicit ScopeTimer(T& targetVariable) noexcept
        : targetVariable_(targetVariable), startTime_(std::chrono::steady_clock::now())
    {
    }

    ~ScopeTimer()
    {
        using std::chrono::duration_cast;
        using std::chrono::microseconds;

        const auto endTime = std::chrono::steady_clock::now();
        const auto delta = endTime - startTime_;
        const auto count = static_cast<T>(duration_cast<microseconds>(delta).count());
        targetVariable_ = count / static_cast<T>(1'000'000);
    }
};

class ScopeTimerPrint
{
   private:
    std::string label_;
    std::chrono::steady_clock::time_point startTime_;

   public:
    explicit ScopeTimerPrint(const std::string& label) noexcept
        : label_(label), startTime_(std::chrono::steady_clock::now())
    {
    }

    ~ScopeTimerPrint()
    {
        using std::chrono::duration_cast;
        using std::chrono::microseconds;

        const auto endTime = std::chrono::steady_clock::now();
        const auto delta = endTime - startTime_;
        const auto count = duration_cast<microseconds>(delta).count();
        std::cout << "[" << label_ << "] " << count / 1'000.0 << " ms" << std::endl;
    }
};
