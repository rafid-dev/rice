#pragma once

#include "types.h"
#include "search.h"

#define MAXHISTORY 8196

void score_moves(Board &board, Movelist &list, SearchStack *ss, SearchInfo &info, Move tt_move);
void score_moves(Board &board, Movelist &list, Move tt_move);
void pickNextMove(const int moveNum, Movelist &list);
void UpdateHistory(Board& board, SearchInfo& info, Move bestmove, Movelist &quietList, int depth);
void UpdateContHistory(Board& board, SearchInfo& info, SearchStack *ss, Move bestmove, Movelist &quietList, int depth);