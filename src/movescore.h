#pragma once

#include "types.h"
#include "search.h"

#define MAXHISTORY 16384
#define MAXCOUNTERHISTORY 16384

void score_moves(SearchThread& st, Movelist &list, SearchStack *ss, Move tt_move);
void score_moves(Board &board, Movelist &list, Move tt_move);

void pick_nextmove(const int moveNum, Movelist &list);

void updateContinuationHistories(SearchStack *ss, Move move, int bonus);

void updateHistories(SearchThread& st, SearchStack *ss, Move bestmove, Movelist &quietList, int depth);

inline int historyBonus(int depth){
    return std::min(depth * depth, 1200);
}

void get_history_scores(int& hus, int& ch, int& fmh, SearchThread& st, SearchStack *ss, const Move move);