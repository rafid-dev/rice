#include "eval.h"
#include "search.h"

int evaluate(SearchThread& st) {
    return st.nnue.Evaluate(st.board.sideToMove);
}