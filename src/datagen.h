#pragma once

#include <cstring>
#include <sstream>
#include <fstream>
#include <thread>
#include "search.h"
#include "misc.h"

struct FenData
{
    std::string fen;
    int eval;
    std::string wdl;
};

void MakeRandomMoves(Board& board);
void generateData(int games, int threadCount);