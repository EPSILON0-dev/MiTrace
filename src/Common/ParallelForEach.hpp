#pragma once

#include <mutex>
#include <thread>
#include <vector>

namespace Common
{

void ParallelForEach(auto begin, auto end, size_t numThreads, auto func)
{
    std::mutex iterLock;
    size_t count = std::distance(begin, end);
    auto it = begin;

    std::vector<std::thread> threads;
    for (size_t i = 0; i < std::min(numThreads, count); ++i)
    {
        threads.emplace_back(
            [&]()
            {
                for (;;)
                {
                    decltype(it) thisIt;
                    {
                        std::scoped_lock<std::mutex> lock(iterLock);
                        if (it == end) break;
                        thisIt = it;
                        it = std::next(it);
                    }
                    func(*thisIt);
                }
            });
    }

    for (auto& thread : threads)
        if (thread.joinable()) thread.join();
}

}  // namespace Common
