#pragma once

#include "types.h"
#include "search.h"

void score_moves(Board &board, Movelist &list, SearchStack *ss, SearchInfo &info, Move tt_move);
void score_moves(Board &board, Movelist &list, Move tt_move);
void pickNextMove(const int moveNum, Movelist &list);
void UpdateHistory(Board& board, SearchInfo& info, Move bestmove, Movelist &quietList, int depth);