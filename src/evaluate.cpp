#include "eval.h"

int evaluate(Board &board) {
    return board.nnue->Evaluate(board.sideToMove);
}