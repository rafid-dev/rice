#pragma once

#include <cstdint>
#include <chrono>

namespace misc {

template<typename Duration = std::chrono::milliseconds>
inline uint64_t tick()
{
    
    return std::chrono::duration_cast<Duration>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

}