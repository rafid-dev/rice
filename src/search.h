#pragma once

#include "types.h"
#include "tt.h"

struct PVTable {
    int length[MAXPLY];
    Move array[MAXPLY][MAXPLY];
};

struct SearchInfo {
    int ply = 0;
    int depth = 0;
    int searchHistory[NPIECES][NSQUARES] = {0};
    
    long nodes = 0l;

    long start_time = 0l;
    long end_time = 0l;
    long stoptimeMax = 0l;
    long stoptimeOpt = 0l;

    bool timeset = false;
    bool stopped = false;
    bool uci = false;

    PVTable pv_table;
};

struct SearchStack {
    int static_eval = 0;

    Move excluded = NO_MOVE;
    Move move = NO_MOVE;
    Move killers[2] = {NO_MOVE, NO_MOVE};
    Move counter = NO_MOVE;
};

struct SearchThreadData{
    SearchInfo *info;
    Board *position;
    TranspositionTable *ttable;
};

extern int RFPMargin;
extern int RFPImprovingBonus;
extern int RFPDepth;
extern int LMRBase;
extern int LMRDivision;

void InitSearch();

void ClearForSearch(SearchInfo& info);

void SearchPosition(Board& board, SearchInfo& info, TranspositionTable *table);
int AlphaBeta(int alpha, int beta, int depth, Board& board, SearchInfo& info, SearchStack *ss, TranspositionTable *table);
int Quiescence(int alpha, int beta, Board &board, SearchInfo &info, SearchStack *ss, TranspositionTable *table);
int AspirationWindowSearch(int prevEval, int depth, Board& board, SearchInfo& info, TranspositionTable *table);