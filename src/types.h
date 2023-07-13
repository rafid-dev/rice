#pragma once

#include "chess.hpp"
#include "nnue.h"

using namespace Chess;

#define NAME "Rice"
#define AUTHOR "Slender"

constexpr int MAXDEPTH = 64;
constexpr int MAXPLY = 64;

enum Score : int{
    ISMATE = 30000,
    KNOWN_WIN = 10000,
    IS_MATE_IN_MAX_PLY = (ISMATE - MAXPLY),
    IS_MATED_IN_MAX_PLY = -IS_MATE_IN_MAX_PLY,
    INF_BOUND = 30001,
    VALUE_NONE = 32002,
};

constexpr int mate_in(int ply){
    return ISMATE - ply;
}

constexpr int mated_in(int ply){
    return -ISMATE + ply;
}

#define IS_DEBUG false

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