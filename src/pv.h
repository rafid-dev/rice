#pragma once

#include "types.h"

struct PV
{
    int length = 0;
    int score = 0;
    Move moves[128];

    void load_from(Move m, const PV &rest);
};