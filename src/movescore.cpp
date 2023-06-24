#include "movescore.h"
#include "eval.h"
#include "see.h"

constexpr int mvv_lva[12][12] = {
    105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605, 104, 204, 304,
    404, 504, 604, 104, 204, 304, 404, 504, 604, 103, 203, 303, 403, 503, 603,
    103, 203, 303, 403, 503, 603, 102, 202, 302, 402, 502, 602, 102, 202, 302,
    402, 502, 602, 101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600,

    105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605, 104, 204, 304,
    404, 504, 604, 104, 204, 304, 404, 504, 604, 103, 203, 303, 403, 503, 603,
    103, 203, 303, 403, 503, 603, 102, 202, 302, 402, 502, 602, 102, 202, 302,
    402, 502, 602, 101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600};

static inline int get_conthist_score(SearchThread& st, SearchStack *ss,
                       const Move& move) {

    int score = 0;

    if ((ss-1)->move){
         score += (ss - 1)->continuationHistory[ss->moved_piece][to(move)];

    }

    if ((ss-2)->move) {
        score += (ss - 2)->continuationHistory[ss->moved_piece][to(move)];
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
static inline void update_ch(SearchThread& st, SearchStack *ss,
                             const Move move, const int score) {

    if ((ss - 1)->move){
        (ss-1)->continuationHistory[ss->moved_piece][to(move)] += score;
    }
    
    if ((ss- 2)->move) {
        (ss-2)->continuationHistory[ss->moved_piece][to(move)] += score;
    }
}

void update_conthist_move(SearchThread& st, SearchStack *ss, Move move, int bonus){
    int score = bonus - get_conthist_score(st, ss, move) * std::abs(bonus) / MAXCOUNTERHISTORY;

    update_ch(st, ss, move, score);
}

void updateHistories(SearchThread& st, SearchStack *ss, Move bestmove, Movelist &quietList, int depth){
    // Update best move score
    int bonus = historyBonus(depth);


    int hist_score = std::min( st.searchHistory[ss->moved_piece][to(bestmove)] + bonus, MAXHISTORY);

    update_conthist_move(st, ss, bestmove, bonus);
    
    st.searchHistory[ss->moved_piece][to(bestmove)] = hist_score;

    for (int i = 0; i < quietList.size; i++) {
        Move move = quietList[i].move;

        if (move == bestmove)
            continue; // Don't give penalty to our best move, so skip it.

        // Penalize moves that didn't cause a beta cutoff.
        int conthist_penalty = bonus - get_conthist_score(st, ss, move) * std::abs(bonus) / MAXCOUNTERHISTORY;
        int hist_penalty = std::max( st.searchHistory[ss->moved_piece][to(move)] - bonus, -MAXHISTORY);


        update_ch(st, ss, move, -conthist_penalty);
        st.searchHistory[ss->moved_piece][to(move)] = hist_penalty;
    }
}

void get_history_scores(int &his, int &ch, int &fmh, SearchThread& st, SearchStack *ss, const Move move) {
    Move previous_move = (ss - 1)->move;
    Move previous_previous_move = (ss - 2)->move;
    Piece moved_piece = ss->moved_piece;

    his = st.searchHistory[ss->moved_piece][to(move)];

    ch = (ss - 1)->continuationHistory.at(moved_piece).at(to(move));

    fmh = (ss - 2)->continuationHistory.at(moved_piece).at(to(move));
}