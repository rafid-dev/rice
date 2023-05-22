#include "search.h"

#include "eval.h"
#include "fancyterminal.h"
#include "misc.h"
#include "movescore.h"
#include "see.h"

#include <cmath>
#include <cstring>
#include <iostream>

int           LMRTable[MAXDEPTH][64];
constexpr int ProbcutDepth = 5;

/* Initialize LMR table using the log formula */
void init_search() {
    float base     = static_cast<float>(static_cast<float>(LMRBase) / 100);
    float division = static_cast<float>(static_cast<float>(LMRDivision) / 100);
    for (int depth = 1; depth < MAXDEPTH; depth++) {
        for (int played = 1; played < 64; played++) {
            LMRTable[depth][played] = base + log(depth) * log(played) / division;
        }
    }
}

// Check if we have to stop the search.
static void check_time(SearchInfo& info) {
    if ((info.timeset && misc::tick() > info.stoptime_max)
        || (info.nodeset && info.nodes_reached >= info.nodes)) {
        info.stopped = true;
    }
}

static bool stop_early(SearchInfo& info) {
    if (info.timeset && (misc::tick() > info.stoptime_opt || info.stopped)) {
        return true;
    } else {
        return false;
    }
}

/* Clear helper variables for search */
void clear_for_search(SearchInfo& info, TranspositionTable* table) {
    info.nodes_reached = 0;
    info.stopped       = false;

    // Reset history
    for (int x = 0; x < 12; x++) {
        for (int i = 0; i < 64; i++) {
            info.searchHistory[x][i] = 0;
        }
    }

    memset(info.contHist.data(), 0, sizeof(info.contHist));

    /* Increment transposition table age */
    table->currentAge++;
}

/* qsearch Search to prevent Horizon Effect.*/
int qsearch(int alpha, int beta, Board& board, SearchInfo& info, SearchStack* ss) {

    /* Checking for time every 2048 nodes */
    if ((info.nodes_reached & 2047) == 0) {
        check_time(info);
    }

    /* We return static evaluation if we exceed max depth */
    if (ss->ply > MAXPLY - 1) {
        return evaluate(board);
    }

    /* Repetition check */
    if (board.isRepetition()) {
        return 0;
    }

    /* standing_pat is our static evaluation of the board */
    int standing_pat = evaluate(board);

    /* if our static evaluation beats beta, we return beta.*/
    if (standing_pat >= beta) {
        return beta;
    }

    /* if our static evaluation betas alpha, we set alpha to standing_pat */
    if (standing_pat > alpha) {
        alpha = standing_pat;
    }

    /* Probe Tranpsosition Table */
    bool     ttHit     = false;
    bool     is_pvnode = (beta - alpha) > 1;
    TTEntry& tte       = table->probe_entry(board.hashKey, ttHit, ss->ply);

    /* Return TT score if we found a TT entry*/
    if (!is_pvnode && ttHit) {
        if ((tte.flag == HFALPHA && tte.score <= alpha) || (tte.flag == HFBETA && tte.score >= beta)
            || (tte.flag == HFEXACT))
            return tte.score;
    }

    /* Move generation */
    /* Generate capture moves for current position*/

    /* Bestscore is our static evaluation in quiscence search */

    int  bestscore  = standing_pat;
    int  move_count = 0;
    int  score      = -INF_BOUND;

    Move bestmove   = NO_MOVE;

    /* Generate moves */
    Movelist list;
    Movegen::legalmoves<CAPTURE>(board, list);

    /* Score moves */
    score_moves(board, list, tte.move);

    /* Moves loop */
    for (int i = 0; i < list.size; i++) {
        /* Pick next move with highest score */
        pick_nextmove(i, list);

        Move move = list[i].move;

        /* SEE pruning in qsearch search */
        /* If we do not SEE a good capture move, we can skip the move.*/
        if (list[i].value < GoodCaptureScore && move_count >= 1) {
            continue;
        }

        /* Make move on board */
        board.makeMove(move);
        table->prefetch_tt(board.hashKey);

        /* Increment ply, nodes and movecount */
        (ss + 1)->ply = ss->ply + 1;
        info.nodes_reached++;
        move_count++;

        ss->move        = move;
        ss->moved_piece = board.pieceAtB(to(move));

        /* Recursive call of qsearch on current position */
        score = -qsearch(-beta, -alpha, board, info, ss + 1);

        /* Undo move on board */
        board.unmakeMove(move);

        /* Return 0 if time is up */
        if (info.stopped) {
            return 0;
        }

        /* If our score beats bestscore, bestmove is move that beat bestscore
         * and bestscore is set to the current score*/
        if (score > bestscore) {
            bestmove  = move;
            bestscore = score;

            if (score > alpha) {
                alpha = score;

                /* Fail soft */
                if (score >= beta) {
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
    table->store(board.hashKey, flag, bestmove, 0, bestscore, standing_pat, ss->ply, is_pvnode);

    /* Return bestscore achieved */
    return bestscore;
}

/* Function based on the Negamax framework and alpha-beta pruning */
/* This is our main Search function , which we use to find "Good moves". */
int negamax(int alpha, int beta, int depth, Board& board, SearchInfo& info, SearchStack* ss) {

    /* Initialize our pv table lenght. */
    /* UNUSED */
    // info.pvTable.length[ss->ply] = ss->ply;

    /* We drop into quiescence search if depth is <= 0 to prevent horizon effect
     * and also end recursion.*/
    if (depth <= 0) {
        return qsearch(alpha, beta, board, info, ss);
    }

    /* Checking for time every 2048 nodes */
    if ((info.nodes_reached & 2047) == 0) {
        check_time(info);
    }

    /* Initialize helper variables */
    bool is_root   = (ss->ply == 0);
    bool in_check  = board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));
    bool is_pvnode = (beta - alpha) > 1;
    int  eval      = 0;
    bool improving = false;

    /* In check extension. We search an extra ply if we are in check. */
    if (in_check) {
        depth++;
    }

    if (!is_root) {
        /* We return static evaluation if we exceed max depth.*/
        if (ss->ply > MAXPLY - 1) {
            return evaluate(board);
        }

        /* Repetition check*/
        if ((board.isRepetition()) && ss->ply) {
            return 0;
        }
    }

    /* Probe transposition table */
    bool     ttHit = false;
    TTEntry& tte   = table->probe_entry(board.hashKey, ttHit, ss->ply);

    if (ss->excluded) {
        ttHit = false;
    }

    /* If we hit a transposition table entry and the depth >= tt entry depth
       and the following conditions meet we will return the tt score. */

    if (!is_pvnode && ttHit && tte.depth >= depth) {
        if ((tte.flag == HFALPHA && tte.score <= alpha) || (tte.flag == HFBETA && tte.score >= beta)
            || (tte.flag == HFEXACT))
            return tte.score;
    }

    /* Set static evaluation and evaluation to our current evaluation of the
     * board*/
    /* We can use the tt entry's evaluation if we have a tt hit so we don't have
     * to re-evaluate from scratch */

    ss->static_eval = eval = ttHit ? tte.eval : evaluate(board);

    /* If we our static evaluation is better than what it was 2 plies ago, we
     * are improving */
    improving = !in_check && ss->static_eval > (ss - 2)->static_eval;

    /* We set static evaluation to 0 if we are in check or if we have hit an
     * excluded move. */
    /* and also set improving to false. */
    if (in_check || ss->excluded) {
        ss->static_eval = eval = 0;
        improving              = false;
        goto movesloop;
    }

    /* Reverse Futility Pruning || Null Move Pruning */
    if (!is_pvnode && !in_check && !is_root && !ss->excluded) {

        /* We can use tt entry's score as score if we hit one.*/
        /* This score is from search so this is more accurate */
        if (ttHit) {
            eval = tte.score;
        }

        /* Reverse Futility Pruning (RFP)
         * If the eval is well above beta by a margin, then we assume the eval
         * will hold above beta.
         */
        if (depth <= RFPDepth && eval - ((depth - improving) * RFPMargin) >= beta) {
            return eval;
        }

        /* Null move pruning
         * If we give our opponent a free move and still maintain beta, we prune
         * some nodes.
         */

        if (eval >= beta && ss->static_eval >= beta && board.nonPawnMat(board.sideToMove)
            && (depth >= 3) && ((ss - 1)->move != NULL_MOVE)) {

            int R = 3 + depth / 3 + std::min(3, (eval - beta) / 180);

            board.makeNullMove();
            ss->move      = NULL_MOVE;

            (ss + 1)->ply = ss->ply + 1;

            int score     = -negamax(-beta, -beta + 1, depth - R, board, info, ss + 1);

            board.unmakeNullMove();

            if (info.stopped) {
                return 0;
            }

            if (score >= beta) {

                /* We don't return mate scores because it can be a false mate.
                 */
                if (score > ISMATE) {
                    score = beta;
                }

                return score;
            }
        }

        // Probcut (~10 elo)
        int rbeta = std::min(beta + 100, ISMATE - MAXPLY - 1);
        if (depth >= ProbcutDepth && abs(beta) < ISMATE
            && (!ttHit || eval >= rbeta || tte.depth < depth - 3)) {
            Movelist list;
            Movegen::legalmoves<CAPTURE>(board, list);
            score_moves(board, list, ss, info, tte.move);
            int score = 0;
            for (int i = 0; i < list.size; i++) {
                pick_nextmove(i, list);
                Move move = list[i].move;

                if (list[i].value < GoodCaptureScore) {
                    continue;
                }

                if (move == tte.move) {
                    continue;
                }

                board.makeMove(move);

                score = -qsearch(-rbeta, -rbeta + 1, board, info, ss);

                if (score >= rbeta) {
                    score = -negamax(-rbeta, -rbeta + 1, depth - 4, board, info, ss);
                }

                board.unmakeMove(move);

                if (score >= rbeta) {
                    table->store(board.hashKey, HFBETA, move, depth - 3, score, ss->static_eval,
                                 ss->ply, is_pvnode);
                    return score;
                }
            }
        }
    }

movesloop:

    /* Initialize variables */
    int  bestscore  = -INF_BOUND;
    int  move_count = 0;
    int  oldAlpha   = alpha;
    int  score      = -INF_BOUND;
    Move bestmove   = NO_MOVE;

    // Move generation
    // Generate all legal moves for current position
    Movelist list;
    Movegen::legalmoves<ALL>(board, list);

    Movelist quietList;      // Quiet moves list
    Movelist captureList;    // Capture moveslist

    // Score moves and pass the tt move so it can be sorted highest
    score_moves(board, list, ss, info, tte.move);

    bool skip_quiet_moves = false;

    // Moves loop
    for (int i = 0; i < list.size; i++) {
        // Pick move with highest possible score
        pick_nextmove(i, list);

        // Initialize move variable
        Move  move        = list[i].move;
        Piece moved_piece = board.pieceAtB(from(move));

        // Skip excluded moves from extension
        if (move == ss->excluded)
            continue;

        bool is_quiet  = (!promoted(move) && !is_capture(board, move));
        int  extension = 0;

        int  h, ch, fh;
        int  history;

        bool refutationMove = (ss->killers[0] == move || ss->killers[1] == move);

        if (is_quiet && skip_quiet_moves) {
            continue;
        }

        if (is_quiet) {
            get_history_scores(h, ch, fh, board, info, ss, move);
            history = h + ch + fh;
        }

        /* Various pruning techniques */
        if (!is_root && bestscore > -ISMATE) {

            // Initialize lmrDepth which we will use soon.
            int lmrDepth = LMRTable[std::min(depth, 63)][std::min(move_count, 63)];

            /* Late Move Pruning/Movecount pruning
                 If we have searched many moves, we can skip the rest. */
            if (is_quiet && !in_check && !is_pvnode && depth <= 5
                && quietList.size >= depth * depth * (2 + 2 * improving)) {
                skip_quiet_moves = true;
                continue;
            }

            // Futility pruning
            if (lmrDepth <= 6 && !in_check && is_quiet && eval + 217 + 71 * depth <= alpha) {
                skip_quiet_moves = true;
            }

            // SEE Pruning
            // Dont search moves at low depths that seem to lose material
            if ((is_quiet ? depth < 6 : depth < 4)
                && !see(board, move, (is_quiet ? -50 * depth : -45 * depth))) {
                continue;
            }
        }

        /* Extensions
         * Search extra ply if move comes from tt
         */
        if (!is_root && depth >= 7 && (move == tte.move) && (tte.flag & HFBETA)
            && abs(tte.score) < ISMATE && tte.depth >= depth - 3) {
            int singular_beta = tte.score - 3 * depth;
            int singularDepth = (depth - 1) / 2;

            ss->excluded      = tte.move;
            int singular_score =
                negamax(singular_beta - 1, singular_beta, singularDepth, board, info, ss);
            ss->excluded = NO_MOVE;

            if (singular_score < singular_beta) {
                extension = 1;
            } else if (singular_beta >= beta) {
                return (singular_beta);    // Multicut
            } else if (tte.score >= beta) {
                extension = -2;
            }
        }

        /* Initialize new depth based on extension*/
        int new_depth = depth + extension;

        /* Make move on current board. */
        board.makeMove(move);
        table->prefetch_tt(board.hashKey);    // TT Prefetch

        /* Set stack move to current move */
        ss->move        = move;
        ss->moved_piece = moved_piece;

        /* Increment ply, nodes and movecount */
        (ss + 1)->ply = ss->ply + 1;

        info.nodes_reached++;
        move_count++;

        /* Add quiet to quiet list if it's a quiet move. */
        if (is_quiet) {
            quietList.Add(move);
        }

        /* A condition for full search.*/
        bool do_fullsearch = do_fullsearch = !is_pvnode || move_count > 1;

        /* Late move reduction
         * Later moves will be searched in a reduced depth.
         * If they beat alpha, It will be researched in a reduced window but
         * full depth.
         */
        // clang-format off
        if (!in_check && depth >= 3 && move_count > (2 + 2 * is_pvnode)) {
            int reduction = LMRTable[std::min(depth, 63)][std::min(63, move_count)];

            reduction += !improving; /* Increase reduction if we're not improving. */
            reduction += !is_pvnode; /* Increase for non pv nodes */
            reduction += is_quiet && !see(board, move, -50 * depth); /* Increase
                                          for quiets and not winning captures */

            // Reduce two plies if it's a counter or killer
            reduction -= refutationMove * 2; 

            /* Adjust the reduction so we don't drop into Qsearch or cause an
             * extension*/
            reduction = std::min(depth - 1, std::max(1, reduction));

            score = -negamax(-alpha - 1, -alpha, new_depth - reduction, board,
                               info, ss + 1);
            

            reduction -= history/12000;

            /* We do a full depth research if our score beats alpha. */
            do_fullsearch = score > alpha && reduction != 1;
        }

        /* Full depth search on a zero window. */
        if (do_fullsearch) {
            score = -negamax(-alpha - 1, -alpha, new_depth - 1, board, info,
                               ss + 1);
        }

        // Principal Variation Search (PVS)
        if (is_pvnode && (move_count == 1 || (score > alpha && score < beta))) {
            score = -negamax(-beta, -alpha, new_depth - 1, board, info, ss + 1);
        }

        // clang-format on

        // Undo move on board
        board.unmakeMove(move);

        if (info.stopped == true && !is_root) {
            return 0;
        }

        if (score > bestscore) {
            bestscore = score;

            if (score > alpha) {
                // Record PV
                alpha    = score;
                bestmove = move;

                // clang-format off
                if (score >= beta) {
                    if (is_quiet) {
                        // Update killers
                        ss->killers[1] = ss->killers[0];
                        ss->killers[0] = move;

                        // Record history score
                        update_hist(board, info, bestmove, quietList, depth);
                        update_conthist(board, info, ss, bestmove, quietList, depth);

                    }
                    break;
                }
                // clang-format on
            }
        }

        // We are safe to break out of the loop if we are sure that we don't
        // return illegal move
        if (info.stopped == true && is_root && info.bestmove != NO_MOVE) {
            break;
        }
    }

    if (!is_root && move_count == 0) {
        if (in_check) {
            return -ISMATE + ss->ply;    // Checkmate
        } else {
            return 0;    // Stalemate
        }
    }

    int flag = bestscore >= beta ? HFBETA : (alpha != oldAlpha) ? HFEXACT : HFALPHA;

    if (ss->excluded == NO_MOVE) {
        table->store(board.hashKey, flag, bestmove, depth, bestscore, ss->static_eval, ss->ply,
                     is_pvnode);
    }

    if (alpha != oldAlpha) {
        info.bestmove = bestmove;
    }

    return bestscore;
}

static inline bool moveExists(Board& board, Move move) {
    Movelist list;
    Movegen::legalmoves<ALL>(board, list);

    if (list.find(move) > -1) {
        return true;
    }

    return false;
}

// Recursive implementation to fetch pv lines.
static void getPvLines(Board& board, std::vector<U64>& positions) {

    if (positions.size() >= MAXPLY) {
        return;
    }

    const auto pvMove = table->probeMove(board.hashKey);

    if (pvMove && moveExists(board, pvMove)) {
        for (auto& pos : positions) {
            if (pos == board.hashKey) {
                return;
            }
        }
        std::cout << " " << convertMoveToUci(pvMove);
        positions.push_back(board.hashKey);
        board.makeMove(pvMove);
        getPvLines(board, positions);
        board.unmakeMove(pvMove);
    }
    return;
}

static void getPvLinesCstr(Board& board, std::vector<U64>& positions) {

    if (positions.size() >= MAXPLY) {
        return;
    }

    const auto pvMove = table->probeMove(board.hashKey);

    if (pvMove && moveExists(board, pvMove)) {
        for (auto& pos : positions) {
            if (pos == board.hashKey) {
                return;
            }
        }
        printf(" %5s", convertMoveToUci(pvMove).c_str());
        positions.push_back(board.hashKey);
        board.makeMove(pvMove);
        getPvLines(board, positions);
        board.unmakeMove(pvMove);
    }
    return;
}

// Explicit template instantiation
template void iterative_deepening<false>(Chess::Board& board, SearchInfo& info);
template void iterative_deepening<true>(Chess::Board& board, SearchInfo& info);

template<bool print_info> void iterative_deepening(Board& board, SearchInfo& info) {
    clear_for_search(info, table);

    nnue->reset_accumulators();
    board.refresh();

    int  score    = 0;

    auto startime = misc::tick();
    Move bestmove = NO_MOVE;

    for (int current_depth = 1; current_depth <= info.depth; current_depth++) {
        score = aspiration_window(score, current_depth, board, info);

        if (info.stopped == true || stop_early(info)) {
            break;
        }
        bestmove   = info.bestmove;
        info.score = score;

        if constexpr (print_info) {
            if (info.uci) {
                auto time_elapsed = misc::tick() - startime;

                std::cout << "info score cp " << score;
                std::cout << " depth " << current_depth;
                std::cout << " nodes " << info.nodes_reached;
                std::cout << " nps "
                          << static_cast<int>(1000.0f * info.nodes_reached / (time_elapsed + 1));
                std::cout << " time " << time_elapsed;
                std::cout << " pv";

                std::vector<uint64_t> positions;
                getPvLines(board, positions);

                std::cout << std::endl;
            } else {
                auto time_elapsed = misc::tick() - startime;

                printf("[%2d/%2d] > eval: %-4.2f nodes: %6.2fM speed: %-5.2f MNPS", current_depth, info.depth,
                       static_cast<float>(score / 100.0f),
                       static_cast<float>(info.nodes_reached / 1000000.0f),
                       static_cast<float>(1000.0f * info.nodes_reached / (time_elapsed + 1))
                           / 1000000.0f);
                           
                std::vector<uint64_t> positions;
                getPvLinesCstr(board, positions);
                std::cout << std::endl;
            }
        }
    }

    if (print_info) {
        std::cout << "bestmove " << convertMoveToUci(bestmove) << std::endl;
    }
}

int aspiration_window(int prevEval, int depth, Board& board, SearchInfo& info) {
    int         score = 0;

    SearchStack stack[MAXPLY + 10], *ss = stack + 7;

    int         delta = 12;

    int         alpha = -INF_BOUND;
    int         beta  = INF_BOUND;

    if (depth > 3) {
        alpha = std::max(-INF_BOUND, prevEval - delta);
        beta  = std::min(INF_BOUND, prevEval + delta);
    }

    while (true) {

        score = negamax(alpha, beta, depth, board, info, ss);

        if (stop_early(info)) {
            break;
        }

        if (score <= alpha) {
            beta  = (alpha + beta) / 2;
            alpha = std::max(-INF_BOUND, score - delta);
        } else if (score >= beta) {
            beta = std::min(score + delta, INF_BOUND);
        } else {
            break;
        }
        delta += delta / 2;

        // std::cout << "DELTA: " << delta << std::endl;
    }

    return score;
}