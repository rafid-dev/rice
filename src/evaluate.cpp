#include "eval.h"
#include "nnue.h"

int evaluate(Board &board) {
    return nnue->Evaluate(board.sideToMove);
}