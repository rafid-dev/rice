#pragma once

#include "pawns.h"
#include "types.h"

extern const int piece_values[12];

extern U64 FileMasks[64];
extern U64 RankMasks[64];
extern U64 IsolatedMasks[64];
extern U64 WhitePassedMasks[64];
extern U64 BlackPassedMasks[64];
extern Score PsqTable[12][64];

extern Score PAWN_PROTECTION[6];

typedef int8_t Phase;

void InitEvaluationMasks();
void InitPsqTables();
int Evaluate(Board &board, PawnTable &pawnTable);
int Evaluate(Board &board);