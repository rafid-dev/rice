#pragma once

#include "chess.hpp"
#include "nnue.h"

using namespace Chess;

#define NAME "Rice"
#define AUTHOR "Slender"

#define MAXPLY 64
#define MAXDEPTH 64

#define INF_BOUND 30000
#define ISMATE 29000

#define IS_DEBUG true

typedef double Time;

static inline bool is_capture(Board &board, Move move) { return (board.pieceAtB(to(move)) != None); }

static inline uint32_t reduce_hash(uint32_t x, uint32_t N) { return ((uint64_t)x * (uint64_t)N) >> 32; }


enum {
    NSQUARES = 64,
    NPIECES = 12,
    NPIECETYPES = 6,
    PvMoveScore = 20000000,
    GoodCaptureScore = 10000000,
    Killer1Score = 9000000,
    Killer2Score = 8000000,
    CounterScore = 6000000,
};