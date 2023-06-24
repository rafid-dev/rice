#pragma once

#include "types.h"
#include "search.h"

#define MAXHISTORY 16384
#define MAXCOUNTERHISTORY 16384

constexpr int mvv_lva[12][12] = {
    105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605, 104, 204, 304,
    404, 504, 604, 104, 204, 304, 404, 504, 604, 103, 203, 303, 403, 503, 603,
    103, 203, 303, 403, 503, 603, 102, 202, 302, 402, 502, 602, 102, 202, 302,
    402, 502, 602, 101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600,

    105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605, 104, 204, 304,
    404, 504, 604, 104, 204, 304, 404, 504, 604, 103, 203, 303, 403, 503, 603,
    103, 203, 303, 403, 503, 603, 102, 202, 302, 402, 502, 602, 102, 202, 302,
    402, 502, 602, 101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600};

void score_moves(SearchThread& st, Movelist &list, SearchStack *ss, Move tt_move);
void score_moves(Board &board, Movelist &list, Move tt_move);

void pick_nextmove(const int moveNum, Movelist &list);

void updateContinuationHistories(SearchStack* ss, Piece piece, Move move, int bonus);
void updateHistories(SearchThread& st, SearchStack *ss, Move bestmove, Movelist &quietList, int depth);

inline int historyBonus(int depth){
   return std::min(2100, 300 * depth - 300);
}

int get_history_scores(int& hus, int& ch, int& fmh, SearchThread& st, SearchStack *ss, const Move move);