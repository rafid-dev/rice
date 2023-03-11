#pragma once

#include "types.h"
#include "search.h"

void score_moves(Board &board, Movelist *list, SearchStack *ss, SearchInfo *info, Move tt_move);
void score_moves(Board &board, Movelist *list);
void pickNextMove(int moveNum, Movelist *list);