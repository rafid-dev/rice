#pragma once

#include <cstdint>
#include <chrono>

namespace misc {

template<typename Duration = std::chrono::milliseconds>
inline double tick()
{
    
    return (double)std::chrono::duration_cast<Duration>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

}