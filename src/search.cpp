#include "search.h"
#include "eval.h"
#include "fancyterminal.h"
#include "misc.h"
#include "movescore.h"
#include "see.h"
#include <cmath>
#include <cstring>
#include <iostream>

int LMRTable[MAXDEPTH][64];
int ProbcutDepth = 5;

/* Initialize LMR table using the log formula */
void InitSearch() {
    float base = static_cast<float>(static_cast<float>(LMRBase) / 100);
    float division = static_cast<float>(static_cast<float>(LMRDivision) / 100);
    for (int depth = 1; depth < MAXDEPTH; depth++) {
        for (int played = 1; played < 64; played++) {
            LMRTable[depth][played] =
                base + log(depth) * log(played) / division;
        }
    }
}

/* Function to check if time has ended and if we have to stop. */
static void CheckUp(SearchInfo &info) {
    if ((info.timeset && GetTimeMs() > info.stoptimeMax) ||
        (info.nodeset && info.nodes >= info.stopNodes)) {
        info.stopped = true;
    }
}

static bool StopEarly(SearchInfo &info) {
    if (info.timeset && (GetTimeMs() > info.stoptimeOpt || info.stopped)) {
        return true;
    } else {
        return false;
    }
}

/* Clear helper variables for search */
void ClearForSearch(SearchInfo &info, TranspositionTable *table) {
    info.nodes = 0;
    info.stopped = false;

    // Reset history
    for (int x = 0; x < 12; x++) {
        for (int i = 0; i < 64; i++) {
            info.searchHistory[x][i] = 0;
        }
    }

    memset(info.contHist.data(), 0, sizeof(info.contHist));
    info.pvTable.clear();

    /* Increment transposition table age */
    table->currentAge++;
}

/* Quiescence Search to prevent Horizon Effect.*/
int Quiescence(int alpha, int beta, Board &board, SearchInfo &info,
               SearchStack *ss) {

    /* Checking for time every 2048 nodes */
    if ((info.nodes & 2047) == 0) {
        CheckUp(info);
    }

    /* We return static evaluation if we exceed max depth */
    if (ss->ply > MAXPLY - 1) {
        return Evaluate(board);
    }

    /* Repetition check */
    if (board.isRepetition()) {
        return 0;
    }

    /* standing_pat is our static evaluation of the board */
    int standing_pat = Evaluate(board);

    /* if our static evaluation beats beta, we return beta.*/
    if (standing_pat >= beta) {
        return beta;
    }

    /* if our static evaluation betas alpha, we set alpha to standing_pat */
    if (standing_pat > alpha) {
        alpha = standing_pat;
    }

    /* Probe Tranpsosition Table */
    bool ttHit = false;
    bool isPvNode = (beta - alpha) > 1;
    TTEntry &tte = table->probeEntry(board.hashKey, ttHit, ss->ply);

    /* Return TT score if we found a TT entry*/
    if (!isPvNode && ttHit) {
        if ((tte.flag == HFALPHA && tte.score <= alpha) ||
            (tte.flag == HFBETA && tte.score >= beta) || (tte.flag == HFEXACT))
            return tte.score;
    }

    /* Move generation */
    /* Generate capture moves for current position*/

    /* Bestscore is our static evaluation in quiscence search */

    int bestscore = standing_pat;
    int moveCount = 0;
    int score = -INF_BOUND;

    Move bestmove = NO_MOVE;

    /* Generate moves */
    Movelist list;
    Movegen::legalmoves<CAPTURE>(board, list);

    /* Score moves */
    score_moves(board, list, tte.move);

    /* Moves loop */
    for (int i = 0; i < list.size; i++) {
        /* Pick next move with highest score */
        pickNextMove(i, list);

        Move move = list.list[i].move;

        /* SEE pruning in Quiescence search */
        /* If we do not SEE a good capture move, we can skip the move.*/
        if (list.list[i].value < GoodCaptureScore && moveCount >= 1) {
            continue;
        }

        /* Make move on board */
        board.makeMove(move);
        table->prefetchTT(board.hashKey);

        /* Increment ply, nodes and movecount */
        (ss + 1)->ply = ss->ply + 1;
        info.nodes++;
        moveCount++;

        ss->move = move;
        ss->movedPiece = board.pieceAtB(to(move));

        /* Recursive call of Quiescence on current position */
        score = -Quiescence(-beta, -alpha, board, info, ss + 1);

        /* Undo move on board */
        board.unmakeMove(move);

        /* Return 0 if time is up */
        if (info.stopped) {
            return 0;
        }

        /* If our score beats bestscore, bestmove is move that beat bestscore
         * and bestscore is set to the current score*/
        if (score > bestscore) {
            bestmove = move;
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
    table->storeEntry(board.hashKey, flag, bestmove, 0, bestscore, standing_pat,
                      ss->ply, isPvNode);

    /* Return bestscore achieved */
    return bestscore;
}

/* Function based on the Negamax framework and alpha-beta pruning */
/* This is our main Search function , which we use to find "Good moves". */
int AlphaBeta(int alpha, int beta, int depth, Board &board, SearchInfo &info,
              SearchStack *ss) {

    /* Initialize our pv table lenght. */
    info.pvTable.length[ss->ply] = ss->ply;

    /* We drop into quiescence search if depth is <= 0 to prevent horizon effect
     * and also end recursion.*/
    if (depth <= 0) {
        return Quiescence(alpha, beta, board, info, ss);
    }

    /* Checking for time every 2048 nodes */
    if ((info.nodes & 2047) == 0) {
        CheckUp(info);
    }

    /* Initialize helper variables */
    bool isRoot = (ss->ply == 0);
    bool inCheck = board.isSquareAttacked(~board.sideToMove,
                                          board.KingSQ(board.sideToMove));
    bool isPvNode = (beta - alpha) > 1;
    int eval = 0;
    bool improving = false;

    /* In check extension. We search an extra ply if we are in check. */
    if (inCheck) {
        depth++;
    }

    if (!isRoot) {
        /* We return static evaluation if we exceed max depth.*/
        if (ss->ply > MAXPLY - 1) {
            return Evaluate(board);
        }

        /* Repetition check*/
        if ((board.isRepetition()) && ss->ply) {
            return 0;
        }
    }

    /* Probe transposition table */
    bool ttHit = false;
    TTEntry &tte = table->probeEntry(board.hashKey, ttHit, ss->ply);

    if (ss->excluded) {
        ttHit = false;
    }

    /* If we hit a transposition table entry and the depth >= tt entry depth
       and the following conditions meet we will return the tt score. */

    if (!isPvNode && ttHit && tte.depth >= depth) {
        if ((tte.flag == HFALPHA && tte.score <= alpha) ||
            (tte.flag == HFBETA && tte.score >= beta) || (tte.flag == HFEXACT))
            return tte.score;
    }

    /* Set static evaluation and evaluation to our current evaluation of the
     * board*/
    /* We can use the tt entry's evaluation if we have a tt hit so we don't have
     * to re-evaluate from scratch */

    ss->static_eval = eval = ttHit ? tte.eval : Evaluate(board);

    /* If we our static evaluation is better than what it was 2 plies ago, we
     * are improving */
    improving = !inCheck && ss->static_eval > (ss - 2)->static_eval;

    /* We set static evaluation to 0 if we are in check or if we have hit an
     * excluded move. */
    /* and also set improving to false. */
    if (inCheck || ss->excluded) {
        ss->static_eval = eval = 0;
        improving = false;
        goto movesloop;
    }

    /* Reverse Futility Pruning || Null Move Pruning */
    if (!isPvNode && !inCheck && !isRoot && !ss->excluded) {

        /* We can use tt entry's score as score if we hit one.*/
        /* This score is from search so this is more accurate */
        if (ttHit) {
            eval = tte.score;
        }

        /* Reverse Futility Pruning (RFP)
         * If the eval is well above beta by a margin, then we assume the eval
         * will hold above beta.
         */
        if (depth <= RFPDepth &&
            eval - ((depth - improving) * RFPMargin) >= beta) {
            return eval;
        }

        /* Null move pruning
         * If we give our opponent a free move and still maintain beta, we prune
         * some nodes.
         */

        if (eval >= beta && ss->static_eval >= beta &&
            board.nonPawnMat(board.sideToMove) && (depth >= 3) &&
            ((ss - 1)->move != NULL_MOVE) && ss->ply >= info.verifPlies) {

            int R = 3 + depth / 3 + std::min(3, (eval - beta) / 180);

            board.makeNullMove();
            ss->move = NULL_MOVE;

            (ss + 1)->ply = ss->ply + 1;

            int score =
                -AlphaBeta(-beta, -beta + 1, depth - R, board, info, ss + 1);

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
        if (depth >= ProbcutDepth && abs(beta) < ISMATE &&
            (!ttHit || eval >= rbeta || tte.depth < depth - 3)) {
            Movelist list;
            Movegen::legalmoves<CAPTURE>(board, list);
            score_moves(board, list, ss, info, tte.move);
            int score = 0;
            for (int i = 0; i < list.size; i++) {
                pickNextMove(i, list);
                Move move = list.list[i].move;

                if (list.list[i].value < GoodCaptureScore) {
                    continue;
                }

                if (move == tte.move) {
                    continue;
                }

                board.makeMove(move);

                score = -Quiescence(-rbeta, -rbeta + 1, board, info, ss);

                if (score >= rbeta) {
                    score = -AlphaBeta(-rbeta, -rbeta + 1, depth - 4, board,
                                       info, ss);
                }

                board.unmakeMove(move);

                if (score >= rbeta) {
                    table->storeEntry(board.hashKey, HFBETA, move, depth - 3,
                                      score, ss->static_eval, ss->ply,
                                      isPvNode);
                    return score;
                }
            }
        }
    }

movesloop:

    /* Initialize variables */
    int bestscore = -INF_BOUND;
    int moveCount = 0;
    int oldAlpha = alpha;
    int score = -INF_BOUND;
    Move bestmove = NO_MOVE;

    // Move generation
    // Generate all legal moves for current position
    Movelist list;
    Movegen::legalmoves<ALL>(board, list);

    Movelist quietList;   // Quiet moves list
    Movelist captureList; // Capture moveslist

    // Score moves and pass the tt move so it can be sorted highest
    score_moves(board, list, ss, info, tte.move);

    bool skipQuiets = false;

    // Moves loop
    for (int i = 0; i < list.size; i++) {
        // Pick move with highest possible score
        pickNextMove(i, list);

        // Initialize move variable
        Move move = list[i].move;
        Piece movedPiece = board.pieceAtB(from(move));

        // Skip excluded moves from extension
        if (move == ss->excluded)
            continue;

        bool isQuiet = (!promoted(move) && !is_capture(board, move));
        int extension = 0;
        int contHistScore =
            isQuiet ? GetConthHistoryScore(board, info, ss, move) : 0;

        int historyScore =
            isQuiet ? info.searchHistory[board.pieceAtB(from(move))][to(move)]
                    : 0;

        int allHistoryScore = contHistScore + historyScore;

        bool refutationMove =
            (ss->killers[0] == move || ss->killers[1] == move);

        if (isQuiet && skipQuiets) {
            continue;
        }

        /* Various pruning techniques */
        if (!isRoot && bestscore > -ISMATE) {

            // Initialize lmrDepth which we will use soon.
            int lmrDepth =
                LMRTable[std::min(depth, 63)][std::min(moveCount, 63)];

            /* Late Move Pruning/Movecount pruning
                 If we have searched many moves, we can skip the rest. */
            if (isQuiet && !inCheck && !isPvNode && depth <= 5 &&
                quietList.size >= depth * depth * (2 + 2 * improving)) {
                skipQuiets = true;
                continue;
            }

            // Futility pruning
            if (lmrDepth <= 6 && !inCheck && isQuiet &&
                eval + 217 + 71 * depth <= alpha) {
                skipQuiets = true;
            }

            // SEE Pruning
            // Dont search moves at low depths that seem to lose material
            if ((isQuiet ? depth < 6 : depth < 4) &&
                !see(board, move, (isQuiet ? -50 * depth : -45 * depth))) {
                continue;
            }
        }

        /* Extensions
         * Search extra ply if move comes from tt
         */
        if (!isRoot && depth >= 7 && (move == tte.move) &&
            (tte.flag & HFBETA) && abs(tte.score) < ISMATE &&
            tte.depth >= depth - 3) {
            int singularBeta = tte.score - 3 * depth;
            int singularDepth = (depth - 1) / 2;

            ss->excluded = tte.move;
            int singularScore = AlphaBeta(singularBeta - 1, singularBeta,
                                          singularDepth, board, info, ss);
            ss->excluded = NO_MOVE;

            if (singularScore < singularBeta) {
                extension = 1;
            } else if (singularBeta >= beta) {
                return (singularBeta); // Multicut
            }
        }

        /* Initialize new depth based on extension*/
        int newDepth = depth + extension;

        /* Make move on current board. */
        board.makeMove(move);
        table->prefetchTT(board.hashKey); // TT Prefetch

        /* Set stack move to current move */
        ss->move = move;
        ss->movedPiece = movedPiece;

        /* Increment ply, nodes and movecount */
        (ss + 1)->ply = ss->ply + 1;

        info.nodes++;
        moveCount++;

        /* Add quiet to quiet list if it's a quiet move. */
        if (isQuiet) {
            quietList.Add(move);
        }

        /* A condition for full search.*/
        bool do_fullsearch = do_fullsearch = !isPvNode || moveCount > 1;

        /* Late move reduction
         * Later moves will be searched in a reduced depth.
         * If they beat alpha, It will be researched in a reduced window but
         * full depth.
         */
        // clang-format off
        if (!inCheck && depth >= 3 && moveCount > (2 + 2 * isPvNode)) {
            int reduction = LMRTable[std::min(depth, 63)][std::min(63, moveCount)];

            reduction += !improving; /* Increase reduction if we're not improving. */
            reduction += !isPvNode; /* Increase for non pv nodes */
            reduction += isQuiet && !see(board, move, -50 * depth); /* Increase
                                          for quiets and not winning captures */

            // Reduce two plies if it's a counter or killer
            reduction -= refutationMove * 2; 

            /* Adjust the reduction so we don't drop into Qsearch or cause an
             * extension*/
            reduction = std::min(depth - 1, std::max(1, reduction));

            score = -AlphaBeta(-alpha - 1, -alpha, newDepth - reduction, board,
                               info, ss + 1);
            

            reduction -= allHistoryScore/12000;

            /* We do a full depth research if our score beats alpha. */
            do_fullsearch = score > alpha && reduction != 1;
        }

        /* Full depth search on a zero window. */
        if (do_fullsearch) {
            score = -AlphaBeta(-alpha - 1, -alpha, newDepth - 1, board, info,
                               ss + 1);
        }

        // Principal Variation Search (PVS)
        if (isPvNode && (moveCount == 1 || (score > alpha && score < beta))) {
            score = -AlphaBeta(-beta, -alpha, newDepth - 1, board, info, ss + 1);
        }

        // clang-format on

        // Undo move on board
        board.unmakeMove(move);

        if (info.stopped == true && !isRoot) {
            return 0;
        }

        if (score > bestscore) {
            bestscore = score;

            if (score > alpha) {
                // Record PV
                alpha = score;
                bestmove = move;

                // clang-format off
                if (score >= beta) {
                    if (isQuiet) {
                        // Update killers
                        ss->killers[1] = ss->killers[0];
                        ss->killers[0] = move;

                        // Record history score
                        UpdateHistory(board, info, bestmove, quietList, depth);
                        UpdateContHistory(board, info, ss, bestmove, quietList, depth);

                    }

                    // TODO: Add capture history.
                    // UpdateCaptureHistory(board, info, move, depth);
                    break;
                }
                // clang-format on
            }
        }

        // We are safe to break out of the loop if we are sure that we don't
        // return illegal move
        if (info.stopped == true && isRoot &&
            info.pvTable.array[0][0] != NO_MOVE) {
            break;
        }
    }

    if (!isRoot && moveCount == 0) {
        if (inCheck) {
            return -ISMATE + ss->ply; // Checkmate
        } else {
            return 0; // Stalemate
        }
    }

    int flag = bestscore >= beta     ? HFBETA
               : (alpha != oldAlpha) ? HFEXACT
                                     : HFALPHA;

    if (ss->excluded == NO_MOVE) {
        table->storeEntry(board.hashKey, flag, bestmove, depth, bestscore,
                          ss->static_eval, ss->ply, isPvNode);
    }

    if (alpha != oldAlpha) {
        info.bestmove = bestmove;
    }

    return bestscore;
}

void SearchPosition(Board &board, SearchInfo &info) {
    ClearForSearch(info, table);

    int score = 0;

    long startime = GetTimeMs();
    Move bestmove = NO_MOVE;

    for (int current_depth = 1; current_depth <= info.depth; current_depth++) {
        score = AspirationWindowSearch(score, current_depth, board, info);

        // score = AlphaBeta(-INF_BOUND, INF_BOUND, current_depth, board, info,
        // ss);

        if (info.stopped == true || StopEarly(info)) {
            break;
        }

        bestmove = info.bestmove;

        std::cout << "info score cp ";
        F_number(score, info.uci, FANCY_Yellow);
        std::cout << " depth ";
        F_number(current_depth, info.uci, FANCY_Green);
        std::cout << " nodes ";
        F_number(info.nodes, info.uci, FANCY_Blue);
        std::cout << " time ";
        F_number((GetTimeMs() - startime), info.uci, FANCY_Cyan);

        // info.pvTable.print();
        std::cout << " pv " << convertMoveToUci(bestmove) << std::endl;
    }

    std::cout << "bestmove " << convertMoveToUci(bestmove) << std::endl;
}

int AspirationWindowSearch(int prevEval, int depth, Board &board,
                           SearchInfo &info) {
    int score = 0;

    SearchStack stack[MAXPLY + 10], *ss = stack + 7;

    int delta = 12;

    int alpha = -INF_BOUND;
    int beta = INF_BOUND;

    if (depth > 3) {
        alpha = std::max(-INF_BOUND, prevEval - delta);
        beta = std::min(INF_BOUND, prevEval + delta);
    }

    while (true) {

        score = AlphaBeta(alpha, beta, depth, board, info, ss);

        if (StopEarly(info)) {
            break;
        }

        if (score <= alpha) {
            beta = (alpha + beta) / 2;
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