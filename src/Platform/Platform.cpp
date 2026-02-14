#include "Platform.hpp"

#include <spdlog/spdlog.h>

using namespace Platform;

void Platform::SetThreadAffinity(std::thread& thread, unsigned coreId)
{
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId % std::thread::hardware_concurrency(), &cpuset);

    int result = pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (result != 0)
    {
        spdlog::error("Failed to set thread affinity: {}", strerror(result));
    }
#else
    (void)thread;
    (void)coreId;
    static bool warned = false;
    if (!warned)
    {
        spdlog::warn("Thread affinity is not implemented on this platform.");
        warned = true;
    }
#endif
}
