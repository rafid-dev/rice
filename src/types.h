#pragma once

#include "chess.hpp"

using namespace Chess;

#define NAME "Rice"
#define AUTHOR "Slender"

#define MAXPLY 64
#define MAXDEPTH 64

#define INF_BOUND 30000
#define ISMATE 29000

static inline bool is_capture(Board& board, Move move){
    return (board.pieceAtB(to(move)) != None);
}

static inline uint32_t reduce_hash(uint32_t x, uint32_t N)
{
    return ((uint64_t)x * (uint64_t)N) >> 32;
}

struct Score{   
    int16_t mg = 0;
    int16_t eg = 0;
    Score(int16_t a, int16_t b){
        mg = a;
        eg = b;
    }
    Score(){
        mg = 0;
        eg = 0;
    }
    Score operator+(const Score x){
        return Score(mg + x.mg, eg + x.eg);
    }
    Score operator-(const Score x){
        return Score(mg - x.mg, eg - x.eg);
    }
    Score operator*(int16_t x){
        return Score(mg*x, eg*x);
    }
    void operator+=(const Score x){
        mg += x.mg;
        eg += x.eg;
    }
    void operator-=(const Score x){
        mg -= x.mg;
        eg -= x.eg;
    }
};

enum {HFNONE, HFBETA, HFALPHA, HFEXACT};

enum {
    NSQUARES = 64,
    GoodCaptureScore = 10000000,
    Killer1Score = 9000000,
    Killer2Score = 8000000
};