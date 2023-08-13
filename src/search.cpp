#include "search.h"

#include "eval.h"
#include "fancyterminal.h"
#include "misc.h"
#include "movescore.h"
#include "see.h"

#include <cmath>
#include <cstring>
#include <iostream>

int lmr_table[MAXDEPTH][64];
int lmp_table[2][8];

constexpr int probcut_depth = 5;

/* Initialize LMR table using the log formula */
void init_search()
{
    float base = LMRBase / 100.0f;
    float division = LMRDivision / 100.0f;
    for (int depth = 1; depth < MAXDEPTH; depth++)
    {
        for (int played = 1; played < 64; played++)
        {
            lmr_table[depth][played] = base + log(depth) * log(played) / division;
        }
    }

    for (int depth = 1; depth < 8; depth++)
    {
        lmp_table[0][depth] = 2.5 + 2 * depth * depth / 4.5;
        lmp_table[1][depth] = 4.0 + 4 * depth * depth / 4.5;
    }
}

int score_to_tt(int score, int ply) {
    if (score >= IS_MATE_IN_MAX_PLY){

        return score - ply;
    }else if (score <= IS_MATED_IN_MAX_PLY){

        return score + ply;
    }

    return score;
}

int score_from_tt(int score, int ply){
    if (score >= IS_MATE_IN_MAX_PLY){

        return score - ply;
    }else if (score <= IS_MATED_IN_MAX_PLY){

        return score + ply;
    }

    return score;
}

/* qsearch Search to prevent Horizon Effect.*/
int qsearch(int alpha, int beta, SearchThread& st, SearchStack *ss)
{

    /* Checking for time every 2048 nodes */
    if (!(st.nodes_reached & 2047))
    {
        st.check_time();
    }

    /* We return static evaluation if we exceed max depth */
    if (ss->ply > MAXPLY - 1)
    {
        return evaluate(st);
    }

    /* Repetition check */
    if (st.board.isRepetition())
    {
        return 0;
    }

    /* standing_pat is our static evaluation of the board */
    int standing_pat = evaluate(st);

    /* if our static evaluation beats beta, we return beta.*/
    if (standing_pat >= beta)
    {
        return beta;
    }

    /* if our static evaluation betas alpha, we set alpha to standing_pat */
    if (standing_pat > alpha)
    {
        alpha = standing_pat;
    }

    /* Probe Tranpsosition Table */
    bool ttHit = false;
    bool is_pvnode = (beta - alpha) > 1;
    TTEntry &tte = table->probe_entry(st.board.hashKey, ttHit, ss->ply);

    const int tt_score = ttHit ? score_from_tt(tte.get_score(), ss->ply) : 0;

    /* Return TT score if we found a TT entry*/
    if (!is_pvnode && ttHit)
    {
        if ((tte.flag == HFALPHA && tt_score <= alpha) || (tte.flag == HFBETA && tt_score >= beta) ||
            (tte.flag == HFEXACT))
            return tt_score;
    }

    /* Move generation */
    /* Generate capture moves for current position*/

    /* Bestscore is our static evaluation in quiscence search */

    int bestscore = standing_pat;
    int move_count = 0;
    int score = -INF_BOUND;

    Move bestmove = NO_MOVE;

    /* Generate moves */
    Movelist list;
    Movegen::legalmoves<CAPTURE>(st.board, list);

    /* Score moves */
    score_moves(st.board, list, tte.move);

    /* Moves loop */
    for (int i = 0; i < list.size; i++)
    {
        /* Pick next move with highest score */
        pick_nextmove(i, list);

        Move move = list[i].move;
        ss->moved_piece = st.board.pieceAtB(to(move));

        /* SEE pruning in qsearch search */
        /* If we do not SEE a good capture move, we can skip the move.*/
        if (list[i].value < GoodCaptureScore && move_count >= 1)
        {
            continue;
        }

        ss->continuationHistory = &st.continuationHistory[ss->moved_piece][to(move)];

        /* Make move on board */
        st.makeMove<true>(move);
        table->prefetch_tt(st.board.hashKey);

        /* Increment ply, nodes and movecount */
        (ss + 1)->ply = ss->ply + 1;
        st.nodes_reached++;
        move_count++;

        ss->move = move;

        /* Recursive call of qsearch on current position */
        score = -qsearch(-beta, -alpha, st, ss + 1);

        /* Undo move on board */
        st.unmakeMove<true>(move);

        /* Return 0 if time is up */
        if (st.info.stopped)
        {
            return 0;
        }

        /* If our score beats bestscore, bestmove is move that beat bestscore
         * and bestscore is set to the current score*/
        if (score > bestscore)
        {
            bestmove = move;
            bestscore = score;

            if (score > alpha)
            {
                alpha = score;

                /* Fail soft */
                if (score >= beta)
                {
                    break;
                }
            }
        }
    }

    /* Transposition Table Entry Flag */
    /* We don't store exact flag in quiescence search, only Beta flag (Lower
     * Bound) and Alpha flag (Upper bound)*/
    int flag = bestscore >= beta ? HFBETA : HFALPHA;

    /* Store transposition table entry */
    table->store(st.board.hashKey, flag, bestmove, 0, score_to_tt(bestscore, ss->ply), standing_pat, ss->ply, is_pvnode);

    /* Return bestscore achieved */
    return bestscore;
}

/* Function based on the Negamax framework and alpha-beta pruning */
/* This is our main Search function , which we use to find "Good moves". */
int negamax(int alpha, int beta, int depth, SearchThread& st, SearchStack *ss, bool cutnode)
{

    /* We drop into quiescence search if depth is <= 0 to prevent horizon effect
     * and also end recursion.*/
    if (depth <= 0)
    {
        return qsearch(alpha, beta, st, ss);
    }

    /* Checking for time every 2048 nodes */
    if ((st.nodes_reached & 2047) == 0)
    {
        st.check_time();
    }

    Board& board = st.board;

    /* Initialize helper variables */
    bool is_root = (ss->ply == 0);
    bool in_check = board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));
    bool is_pvnode = (beta - alpha) > 1;
    bool improving = false;
    int eval = 0;

    /* In check extension. We search an extra ply if we are in check. */
    if (in_check)
    {
        depth++;
    }

    if (!is_root)
    {
        /* We return static evaluation if we exceed max depth.*/
        if (ss->ply > MAXPLY - 1)
        {
            return evaluate(st);
        }

        /* Repetition check*/
        if ((board.isRepetition()) && ss->ply)
        {
            return 0;
        }

        alpha = std::max(alpha, mated_in(ss->ply));
		beta = std::min(beta, mate_in(ss->ply + 1));
		if (alpha >= beta){
			return alpha;
        }
    }

    /* Probe transposition table */
    bool ttHit = false;
    TTEntry &tte = table->probe_entry(board.hashKey, ttHit, ss->ply);

    const Move excluded_move = ss->excluded;
    const int tt_score = ttHit ? score_from_tt(tte.get_score(), ss->ply) : 0;

    if (excluded_move)
    {
        ttHit = false;
    }

    /* If we hit a transposition table entry and the depth >= tt entry depth
       and the following conditions meet we will return the tt score. */

    if (!is_pvnode && ttHit && tte.depth >= depth)
    {
        if ((tte.flag == HFALPHA && tt_score <= alpha) || (tte.flag == HFBETA && tt_score >= beta) ||
            (tte.flag == HFEXACT))
            return tt_score;
    }

    /* Set static evaluation and evaluation to our current evaluation of the
     * board*/
    /* We can use the tt entry's evaluation if we have a tt hit so we don't have
     * to re-evaluate from scratch */

    ss->static_eval = eval = ttHit ? tte.get_eval() : evaluate(st);

    /* If we our static evaluation is better than what it was 2 plies ago, we
     * are improving */
    improving = !in_check && ss->static_eval > (ss - 2)->static_eval;

    /* We set static evaluation to 0 if we are in check or if we have hit an
     * excluded move. */
    /* and also set improving to false. */
    if (in_check || excluded_move)
    {
        ss->static_eval = eval = 0;
        improving = false;
    }

    /* Reverse Futility Pruning || Null Move Pruning */
    if (!is_pvnode && !in_check && !is_root && !excluded_move)
    {

        /* We can use tt entry's score as score if we hit one.*/
        /* This score is from search so this is more accurate */
        if (ttHit)
        {
            eval = tt_score;
        }

        /* Reverse Futility Pruning (RFP)
         * If the eval is well above beta by a margin, then we assume the eval
         * will hold above beta.
         */
        if (depth < 9 && eval >= beta && eval - ((depth - improving) * 77) - (ss - 1)->stat_score/400 >= beta)
        {
            return eval;
        }

        /* Null move pruning
         * If we give our opponent a free move and still maintain beta, we prune
         * some nodes.
         */

        if (ss->static_eval >= (beta - 76 * improving) && board.nonPawnMat(board.sideToMove) && (depth >= 3) &&
            ((ss - 1)->move != NULL_MOVE) && (!ttHit || tte.flag != HFALPHA | eval >= beta))
        {

            int R = 3 + depth / 3 + std::min(3, (eval - beta) / 180);

            ss->continuationHistory = &st.continuationHistory[None][0];

            board.makeNullMove();
            ss->move = NULL_MOVE;

            (ss + 1)->ply = ss->ply + 1;

            int score = -negamax(-beta, -beta + 1, depth - R, st, ss + 1, !cutnode);

            board.unmakeNullMove();

            if (st.info.stopped)
            {
                return 0;
            }

            if (score >= beta)
            {

                /* We don't return mate scores because it can be a false mate.
                 */
                if (score > ISMATE)
                {
                    score = beta;
                }

                return score;
            }
        }

        // Probcut
        int rbeta = std::min(beta + 100, ISMATE - MAXPLY - 1);
        if (depth >= probcut_depth && abs(beta) < ISMATE && (!ttHit || eval >= rbeta || tte.depth < depth - 3))
        {
            Movelist list;
            Movegen::legalmoves<CAPTURE>(board, list);
            score_moves(st, list, ss, tte.move);
            int score = 0;
            for (int i = 0; i < list.size; i++)
            {
                pick_nextmove(i, list);
                Move move = list[i].move;
                ss->moved_piece = board.pieceAtB(from(move));

                if (list[i].value < GoodCaptureScore)
                {
                    continue;
                }

                if (move == tte.move)
                {
                    continue;
                }

                ss->continuationHistory = &st.continuationHistory[ss->moved_piece][to(move)];

                st.makeMove<true>(move);

                score = -qsearch(-rbeta, -rbeta + 1, st, ss);

                if (score >= rbeta)
                {
                    score = -negamax(-rbeta, -rbeta + 1, depth - 4, st, ss, !cutnode);
                }

                st.unmakeMove<true>(move);

                if (score >= rbeta)
                {
                    table->store(board.hashKey, HFBETA, move, depth - 3, score, ss->static_eval, ss->ply, is_pvnode);
                    return score;
                }
            }
        }

        // Razoring
        if (eval - 63 + 182 * depth <= alpha){
            return qsearch(alpha, beta, st, ss);
        }
    }

    if (cutnode && depth >= 7 && tte.move == NO_MOVE) 
        depth--;

    /* Initialize variables */
    int bestscore = -INF_BOUND;
    int move_count = 0;
    int oldAlpha = alpha;
    int score = -INF_BOUND;
    Move bestmove = NO_MOVE;

    // Move generation
    // Generate all legal moves for current position
    Movelist list;
    Movegen::legalmoves<ALL>(board, list);

    Movelist quietList;   // Quiet moves list

    // Score moves and pass the tt move so it can be sorted highest
    score_moves(st, list, ss, tte.move);

    bool skip_quiet_moves = false;

    // Moves loop
    for (int i = 0; i < list.size; i++)
    {
        // Pick move with highest possible score
        pick_nextmove(i, list);

        // Initialize move variable
        Move move = list[i].move;
        Piece moved_piece = board.pieceAtB(from(move));
        ss->moved_piece = moved_piece;

        // Skip excluded moves from extension
        if (move == excluded_move)
            continue;

        bool is_quiet = (!promoted(move) && !is_capture(board, move));
        int extension = 0;
        
        bool refutationMove = (ss->killers[0] == move || ss->killers[1] == move);

        int hist = 0, counter_hist = 0, follow_up_hist = 0;
        int history = get_history_scores(hist, counter_hist, follow_up_hist, st, ss, move);

        ss->stat_score = 2 * hist + counter_hist + follow_up_hist - 4000;

        if (is_quiet && skip_quiet_moves)
        {
            continue;
        }

        /* Various pruning techniques */
        if (!is_root && bestscore > -ISMATE)
        {

            // Initialize lmrDepth which we will use soon.
            int lmrDepth = lmr_table[std::min(depth, 63)][std::min(move_count, 63)];

            // Pruning for quiets

            if (is_quiet)
            {
                // Late Move Pruning/Movecount pruning
                // If we have searched many moves, we can skip the rest.
                if (!in_check && !is_pvnode && depth <= 7 && quietList.size >= lmp_table[improving][depth])
                {
                    skip_quiet_moves = true;
                    continue;
                }

                // Continuation pruning
                if (lmrDepth < 3 && history < -4000 * depth){
                    continue;
                }

                // Futility pruning
                if (lmrDepth <= 6 && !in_check && eval + 217 + 71 * depth <= alpha)
                {
                    skip_quiet_moves = true;
                }

                // See pruning for quiets
                if (depth <= 8 && !see(board, move, -70 * depth))
                {
                    continue;
                }
            }
            else
            {
                // See pruning for noisy
                if (depth <= 6 && !see(board, move, -15 * depth * depth))
                {
                    continue;
                }

            }
        }

        /* Extensions
         * Search extra ply if move comes from tt
         */
        if (!is_root && depth >= (6 + is_pvnode) && (move == tte.move) && (tte.flag & HFBETA) && abs(tt_score) < ISMATE &&
            tte.depth >= depth - 3)
        {

            int singular_beta = tt_score - depth;
            int singularDepth = (depth - 1) / 2;

            ss->excluded = tte.move;
            int singular_score = negamax(singular_beta - 1, singular_beta, singularDepth, st, ss, cutnode);
            ss->excluded = NO_MOVE;

            if (singular_score < singular_beta)
            {
                extension = 1;

                if (!is_pvnode && singular_score < singular_beta - 20 && ss->double_extensions <= 5)
                {
						extension = 2;
						ss->double_extensions = (ss - 1)->double_extensions + 1;
                }
            }
            else if (singular_beta >= beta)
            {
                return (singular_beta); // Multicut
            }
            else if (tt_score >= beta)
            {
                extension = -2;
            }else if (tt_score <= singular_score){
                extension = -1;
            }else if (cutnode){
                extension = -1;
            }
        }

        /* Initialize new depth based on extension*/
        int new_depth = depth + extension;

        ss->continuationHistory = &st.continuationHistory[ss->moved_piece][to(move)];

        /* Make move on current board. */
        st.makeMove<true>(move);
        table->prefetch_tt(board.hashKey); // TT Prefetch

        /* Set stack move to current move */
        ss->move = move;

        /* Increment ply, nodes and movecount */
        (ss + 1)->ply = ss->ply + 1;

        st.nodes_reached++;
        move_count++;

        if (is_root && depth == 1 && move_count == 1){
            st.bestmove = move;   
        }

        /* Add quiet to quiet list if it's a quiet move. */
        if (is_quiet)
        {
            quietList.Add(move);
        }

        /* A condition for full search.*/
        bool do_fullsearch = !is_pvnode || move_count > 1;

        /* Late move reduction
         * Later moves will be searched in a reduced depth.
         * If they beat alpha, It will be researched in a reduced window but
         * full depth.
         */
        // clang-format off
        if (!in_check && depth >= 3 && move_count > (2 + 2 * is_pvnode)) {
            int reduction = lmr_table[std::min(depth, 63)][std::min(63, move_count)];

            reduction += !improving; /* Increase reduction if we're not improving. */
            reduction += !is_pvnode; /* Increase for non pv nodes */
            reduction += is_quiet && !see(board, move, -50 * depth); /* Increase for quiet moves that lose material */

            // Reduce two plies if it's a counter or killer
            reduction -= refutationMove * 2; 

            // Reduce or Increase according to history score
            reduction -= history/4000;

            /* Adjust the reduction so we don't drop into Qsearch or cause an
             * extension*/
            reduction = std::min(depth - 1, std::max(1, reduction));

            score = -negamax(-alpha - 1, -alpha, new_depth - reduction, st, ss + 1, true);

            /* We do a full depth research if our score beats alpha. */
            do_fullsearch = score > alpha && reduction != 1;

            bool deeper = score > bestscore + 70 + 12 * (new_depth - reduction);

            new_depth += deeper;
        }

        /* Full depth search on a zero window. */
        if (do_fullsearch) {
            score = -negamax(-alpha - 1, -alpha, new_depth - 1, st, ss + 1, !cutnode);
        }

        // Principal Variation Search (PVS)
        if (is_pvnode && (move_count == 1 || (score > alpha && score < beta))) {
            score = -negamax(-beta, -alpha, new_depth - 1, st, ss + 1, false);
        }

        // clang-format on

        // Undo move on board
        st.unmakeMove<true>(move);

        if (st.info.stopped && !is_root)
        {
            return 0;
        }

        if (score > bestscore)
        {
            bestscore = score;

            if (score > alpha)
            {
                // Record PV
                alpha = score;
                bestmove = move;

                // clang-format off
                if (score >= beta) {
                    if (is_quiet) {
                        // Update killers
                        ss->killers[1] = ss->killers[0];
                        ss->killers[0] = move;

                        // Update histories
                        updateHistories(st, ss, bestmove, quietList, depth);
                    }
                    break;
                }
                // clang-format on
            }
        }

        // We are safe to break out of the loop if we are sure that we don't
        // return illegal move
        if (st.info.stopped && is_root && st.bestmove != NO_MOVE)
        {
            break;
        }
    }

    if (move_count == 0) bestscore = in_check ? mated_in(ss->ply) : 0;

    int flag = bestscore >= beta ? HFBETA : (alpha != oldAlpha) ? HFEXACT
                                                                : HFALPHA;

    if (excluded_move == NO_MOVE)
    {
        table->store(board.hashKey, flag, bestmove, depth, score_to_tt(bestscore, ss->ply), ss->static_eval, ss->ply, is_pvnode);
    }

    if (alpha != oldAlpha)
    {
        st.bestmove = bestmove;
    }

    return bestscore;
}

static inline bool moveExists(Board &board, Move move)
{
    Movelist list;
    Movegen::legalmoves<ALL>(board, list);

    if (list.find(move) > -1)
    {
        return true;
    }

    return false;
}

// Recursive implementation to fetch pv lines.
static void getPvLines(SearchThread& st, std::vector<U64> &positions, Move bestmove = NO_MOVE)
{

    if (positions.size() >= MAXPLY)
    {
        return;
    }

    if (bestmove != NO_MOVE){
        std::cout << " " << convertMoveToUci(bestmove);
        positions.push_back(st.board.hashKey);
        st.makeMove<false>(bestmove);
        getPvLines(st, positions);
        st.unmakeMove<false>(bestmove);

        return;
    }

    auto pvMove = table->probeMove(st.board.hashKey);

    if (pvMove && moveExists(st.board, pvMove))
    {
        for (auto &pos : positions)
        {
            if (pos == st.board.hashKey)
            {
                return;
            }
        }
        std::cout << " " << convertMoveToUci(pvMove);
        positions.push_back(st.board.hashKey);

        st.makeMove<false>(pvMove);
        getPvLines(st, positions);
        st.unmakeMove<false>(pvMove);
    }
    return;
}

// Explicit template instantiation
template void iterative_deepening<false>(SearchThread& st);
template void iterative_deepening<true>(SearchThread& st);

template <bool print_info>
void iterative_deepening(SearchThread& st)
{
    SearchInfo& info = st.info;

    st.clear();
    st.initialize();
    table->nextAge();
    info.totalNodes = 0;

    int score = 0;

    auto startime = st.start_time();
    Move bestmove = NO_MOVE;

    for (int current_depth = 1; current_depth <= info.depth; current_depth++)
    {
        score = aspiration_window(score, current_depth, st, bestmove);

        if (st.info.stopped || st.stop_early())
        {
            break;
        }
        
        bestmove = st.bestmove;
        info.score = score;
        
        info.totalNodes += st.nodes_reached;
        st.nodes_reached = 0;

        if (info.timeset)
        {
            st.tm.update_tm(bestmove);
        }
        
        if constexpr (print_info)
        {
                auto time_elapsed = misc::tick() - startime;

                std::cout << "info score ";

                if (score >= ISMATED && score <= IS_MATED_IN_MAX_PLY){
                    std::cout << "mate " << ((ISMATED - score)/2);
                }else if(score >= IS_MATE_IN_MAX_PLY && score <= ISMATE){
                    std::cout << "mate " << ((ISMATE - score)/2);
                }else{
                    std::cout << "cp "<< score;
                }
                
                std::cout << " depth " << current_depth;
                std::cout << " nodes " << st.info.totalNodes;
                std::cout << " nps " << static_cast<int>(1000.0f * st.info.totalNodes / (time_elapsed + 1));
                std::cout << " time " << static_cast<uint64_t>(time_elapsed);
                std::cout << " pv";

                std::vector<uint64_t> positions;
                getPvLines(st, positions, bestmove);

                std::cout << std::endl;
            

        }
    }

    if (print_info)
    {
        std::cout << "bestmove " << convertMoveToUci(bestmove) << std::endl;
    }
}

int aspiration_window(int prevEval, int depth, SearchThread& st, Move& bestmove)
{
    int score = 0;

    SearchStack stack[MAXPLY + 10], *ss = stack + 7;

    int delta = 12;

    int alpha = -INF_BOUND;
    int beta = INF_BOUND;

    int initial_depth = depth;

    if (depth > 3)
    {
        alpha = std::max<int>(-INF_BOUND, prevEval - delta);
        beta = std::min<int>(INF_BOUND, prevEval + delta);
    }

    while (true)
    {

        score = negamax(alpha, beta, depth, st, ss, false);

        if (st.stop_early())
        {
            break;
        }

        if (score <= alpha)
        {
            beta = (alpha + beta) / 2;
            alpha = std::max<int>(-INF_BOUND, score - delta);

            depth = initial_depth;
        }
        else if (score >= beta)
        {
            beta = std::min<int>(score + delta, INF_BOUND);
            if (abs(score) <= ISMATE / 2 && depth > 1)
            {
                depth--;
            }

            // Idea from StockDory
            bestmove = st.bestmove;
        }
        else
        {
            break;
        }
        delta += delta / 2;
    }

    return score;
}
