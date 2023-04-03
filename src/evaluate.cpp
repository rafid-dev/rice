#include "eval.h"

int Evaluate(Board &board) {
    return nnue.Evaluate(board.sideToMove);
}