#pragma once

#include "types.h"

// 8192 GBS
#define MAXHASH 8192

struct TTEntry {
    int16_t score = 0;
    int16_t eval = 0;

    uint8_t flag = HFNONE;
    uint8_t depth = 0;
    uint8_t age = 0;

    Move move = NO_MOVE;
    U64 key = 0ULL;
};

class TranspositionTable {
  private:
    std::vector<TTEntry> entries;

  public:
    int currentAge = 0;
    void Initialize(int usersize);
    void storeEntry(U64 key, uint8_t f, Move move, uint8_t depth, int16_t score,
                    int16_t eval, int ply, bool pv);
    TTEntry &probeEntry(U64 key, bool &ttHit, int ply);
    void prefetchTT(const U64 key);
    void clear();
};

static inline void prefetch(const void *addr) {
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
    _mm_prefetch((char *)addr, _MM_HINT_T0);
#else
    __builtin_prefetch(addr);
#endif
}