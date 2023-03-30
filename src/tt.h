#pragma once

#include <map>
#include "types.h"

// 8192 GBS
#define MAXHASH 8192

struct TTEntry {
    int depth = 0;
    int score = 0;
    int eval = 0;
    int flag = HFNONE;
    int age = 0;
    bool pv = false;
    Move move = NO_MOVE;
    U64 key = 0ULL;
};

class TranspositionTable {
    private:
    std::vector<TTEntry>entries;

    public:
    int currentAge = 0;
    void Initialize(int usersize);
    void storeEntry(U64 key, int f, Move move, int depth, int score, int eval, int ply, bool pv);
    bool probeEntry(U64 key, TTEntry *entry, int ply);
    void prefetchTT(const U64 key);
    void clear();
};