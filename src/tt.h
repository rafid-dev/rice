#pragma once

#include "types.h"

// 8192 MBS
#define MAXHASH 8192

using TTKey = uint16_t;

enum : uint8_t { HFNONE, HFBETA, HFALPHA, HFEXACT };

struct TTEntry {
    int16_t score = 0;
    int16_t eval = 0;

    uint8_t flag : 2;
    uint8_t age : 6;

    uint8_t depth = 0;

    Move move = NO_MOVE;
    TTKey key = 0;

    int get_score(){
      return (int)score;
    }

    int get_eval(){
      return (int)eval;
    }
};

class TranspositionTable {
  private:
    std::vector<TTEntry> entries;

  public:
    uint8_t currentAge = 0;
    void Initialize(int usersize);
    void store(U64 key, uint8_t f, Move move, uint8_t depth, int score, int eval, int ply, bool pv);
    TTEntry &probe_entry(U64 key, bool &ttHit, int ply);
    Move probeMove(U64 key);
    void prefetch_tt(const U64 key);
    void clear();
};

static inline void prefetch(const void *addr) {
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
    _mm_prefetch((char *)addr, _MM_HINT_T0);
#else
    __builtin_prefetch(addr);
#endif
}