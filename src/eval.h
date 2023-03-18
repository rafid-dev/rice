#pragma once

#include "types.h"
#include "pawns.h"

extern const int piece_values[12];

extern U64 FileMasks[64];
extern U64 RankMasks[64];
extern U64 IsolatedMasks[64];
extern U64 WhitePassedMasks[64];
extern U64 BlackPassedMasks[64];

void InitEvaluationMasks();
void InitPestoTables();
int Evaluate(Board& board, PawnTable& pawnTable);
int Evaluate(Board& board);
// parameters_t get_initial_parameters();
// int get_fen_eval_result(const std::string& fen); 
// void print_parameters(const parameters_t& parameters);