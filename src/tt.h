#pragma once

#include "types.h"

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
    void clear();
};