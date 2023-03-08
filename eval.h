#pragma once

#include "types.h"

extern const int piece_values[12];

void InitPestoTables();
int Evaluate(Board& board);