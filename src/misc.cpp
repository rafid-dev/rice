#include <chrono>
#include "misc.h"

long GetTimeMs() {
	std::chrono::milliseconds sec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	
	return sec.count();
}