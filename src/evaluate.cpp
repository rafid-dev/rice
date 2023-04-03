#include "eval.h"

using namespace std::chrono;

int Evaluate(Board &board) {
    return nnue.Evaluate(board.sideToMove);
}