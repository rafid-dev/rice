#pragma once

#include "types.h"

// For indexing middle game and endgame values
#define MIDDLEGAME 0
#define ENDGAME 1

extern const int piece_values[12];

void InitEvaluationMasks();
void InitPestoTables();

int Evaluate(Board& board);
// parameters_t get_initial_parameters();
// int get_fen_eval_result(const std::string& fen);
// void print_parameters(const parameters_t& parameters);