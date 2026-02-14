#pragma once

#include <thread>

namespace Platform
{
void SetThreadAffinity(std::thread&thread, unsigned coreId);
}
