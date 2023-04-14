#include "eval.h"

int Evaluate(Board &board) {
    return board.nnue->Evaluate(board.sideToMove);
}