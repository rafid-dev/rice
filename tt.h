#pragma once

#include "types.h"

enum TTFlag {HFNONE, HFEXACT, HFBETA, HFALPHA};

struct TTEntry {
    int depth = 0;
    int score = 0;
    int eval = 0;
    TTFlag flag = HFNONE;
    Move move = NO_MOVE;
};

