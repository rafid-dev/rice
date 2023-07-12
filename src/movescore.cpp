#include "movescore.h"
#include "eval.h"
#include "see.h"


static inline int get_conthist_score(SearchThread& st, SearchStack *ss,
                       const Move& move) {

    int score = 0;

    if ((ss-1)->move){
         score += (*(ss - 1)->continuationHistory)[st.board.pieceAtB(from(move))][to(move)];

    }

    if ((ss-2)->move) {
        score += (*(ss - 2)->continuationHistory)[st.board.pieceAtB(from(move))][to(move)];
    }

    return score;
}

// Move scoring
void score_moves(SearchThread& st, Movelist &list, SearchStack *ss, Move tt_move) {

    // Loop through moves in movelist.
    for (int i = 0; i < list.size; i++) {
        Piece victim = st.board.pieceAtB(to(list[i].move));
        Piece attacker = st.board.pieceAtB(from(list[i].move));

        // Score tt move the highest
        if (list[i].move == tt_move) {
            list[i].value = PvMoveScore;
        }else if (promoted(list[i].move) && piece(list[i].move) == QUEEN){
            list[i].value = GoodCaptureScore + 5000;
        } else if (victim != None) {
            // If it's a capture move, we score using MVVLVA (Most valuable
            // victim, Least Valuable Attacker) and if see move that doesn't
            // lose material, we add additional bonus

            list[i].value = mvv_lva[attacker][victim] +
                            (GoodCaptureScore * see(st.board, list[i].move, -107));
        } else if (list[i].move == ss->killers[0]) {
            // Score for killer 1
            list[i].value = Killer1Score;
        } else if (list[i].move == ss->killers[1]) {
            // Score for killer 2
            list[i].value = Killer2Score;
        } else {
            // Otherwise, history score.
            list[i].value = st.searchHistory[attacker][to(list[i].move)] + get_conthist_score(st, ss, list[i].move);
        }
    }
}

// Used for Qsearch move scoring
void score_moves(Board &board, Movelist &list, Move tt_move) {

    // Loop through moves in movelist.
    for (int i = 0; i < list.size; i++) {
        Piece victim = board.pieceAtB(to(list[i].move));
        Piece attacker = board.pieceAtB(from(list[i].move));
        if (list[i].move == tt_move) {
            list[i].value = 20000000;
        } else if (victim != None) {
            // If it's a capture move, we score using MVVLVA (Most valuable
            // victim, Least Valuable Attacker)
            list[i].value = mvv_lva[attacker][victim] +
                            (GoodCaptureScore * see(board, list[i].move, -107));
        }
    }
}

void pick_nextmove(const int moveNum, Movelist &list) {

    ExtMove temp;
    int index = 0;
    int bestscore = -INF_BOUND;
    int bestnum = moveNum;

    for (index = moveNum; index < list.size; ++index) {

        if (list[index].value > bestscore) {
            bestscore = list[index].value;
            bestnum = index;
        }
    }

    temp = list[moveNum];
    list[moveNum] = list[bestnum]; // Sort the highest score move to highest.
    list[bestnum] = temp;
}


void updateH(int16_t& historyScore, const int bonus)
{
    historyScore += bonus - historyScore * std::abs(bonus) / MAXHISTORY;
}

void updateCH(int16_t& historyScore, const int bonus) {
    
    historyScore += bonus - historyScore * std::abs(bonus) / MAXCOUNTERHISTORY;
}

void updateContinuationHistories(SearchStack* ss, Piece piece, Move move, int bonus){

    if ((ss - 1)->move){
        updateCH((*(ss - 1)->continuationHistory)[ss->moved_piece][to(move)], bonus);
    }

    if ((ss - 2)->move){
        updateCH((*(ss - 2)->continuationHistory)[ss->moved_piece][to(move)], bonus);   
    }
}

void updateHistories(SearchThread& st, SearchStack *ss, Move bestmove, Movelist &quietList, int depth){
    // Update best move score
    int bonus = historyBonus(depth);

    if (depth > 2){
        updateH(st.searchHistory[st.board.pieceAtB(from(bestmove))][to(bestmove)], bonus);
        updateContinuationHistories(ss, st.board.pieceAtB(from(bestmove)), bestmove, bonus);
    }

    for (int i = 0; i < quietList.size; i++) {
        Move move = quietList[i].move;

        if (move == bestmove)
            continue; // Don't give penalty to our best move, so skip it.

        // Penalize moves that didn't cause a beta cutoff.
        updateContinuationHistories(ss, st.board.pieceAtB(from(move)), move, -bonus);
        updateH(st.searchHistory[st.board.pieceAtB(from(move))][to(move)], -bonus);
    }
}

int get_history_scores(int &his, int &ch, int &fmh, SearchThread& st, SearchStack *ss, const Move move) {
    Move previous_move = (ss - 1)->move;
    Move previous_previous_move = (ss - 2)->move;
    Piece moved_piece = st.board.pieceAtB(from(move));

    his = st.searchHistory[st.board.pieceAtB(from(move))][to(move)];

    ch = (ss-1)->move ? (*(ss - 1)->continuationHistory)[moved_piece][to(move)] : 0;
    fmh = (ss-2)->move ? (*(ss - 2)->continuationHistory)[moved_piece][to(move)] : 0;

    return his + ch + fmh;
}