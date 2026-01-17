#pragma once

#include <chrono>

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
