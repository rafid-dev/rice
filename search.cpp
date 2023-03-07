#include <iostream>
#include "search.h"
#include "eval.h"
#include "misc.h"
#include "movescore.h"

static void CheckUp(SearchInfo &info)
{
    if (info.timeset == true && GetTimeMs() > info.end_time)
    {
        info.stopped = true;
    }
}

void ClearForSearch(SearchInfo &info)
{
    info.nodes = 0;
    info.stopped = false;

    // Reset history
    for (int x = 0; x < 12; x++)
    {
        for (int i = 0; i < 64; i++)
        {
            info.searchHistory[x][i] = 0;
        }
    }
}

/* Quiescence Search to prevent Horizon Effect.*/
int Quiescence(int alpha, int beta, Board &board, SearchInfo &info, SearchStack *ss)
{
    /* Checking for time every 2048 nodes */
    if ((info.nodes & 2047) == 0)
    {
        CheckUp(info);
    }

    /* We return static evaluation if we exceed max depth */
    if (info.ply > MAXPLY - 1)
    {
        return Evaluate(board);
    }

    /* Repetition check */
    if (board.isRepetition())
    {
        return 0;
    }

    int score = Evaluate(board);
    if (score >= beta)
    {
        return beta;
    }
    if (score > alpha)
    {
        alpha = score;
    }

    /* Move generation */
    /* Generate capture moves for current position*/

    int bestscore = score;
    score = -INF_BOUND;

    Movelist list;
    Movegen::legalmoves<CAPTURE>(board, list);

    score_moves(board, &list);

    /* Move loop */
    for (int i = 0; i < list.size; i++)
    {
        pickNextMove(i, &list);

        Move move = list.list[i].move;

        // Make the move on board
        board.makeMove(move);
        // Increment ply and nodes
        info.ply++;
        info.nodes++;

        // Call Quiescence on current position
        ss->move = move;
        score = -Quiescence(-beta, -alpha, board, info, ss + 1);
        // Undo move on board
        board.unmakeMove(move);
        // Decrement ply
        info.ply--;

        if (info.stopped)
        {
            return 0;
        }

        if (score > bestscore)
        {
            bestscore = score;
            if (score > alpha)
            {
                alpha = score;
                if (score >= beta)
                {
                    break;
                }
            }
        }
    }
    return bestscore;
}

int AlphaBeta(int alpha, int beta, int depth, Board &board, SearchInfo &info, SearchStack *ss)
{

    // init pv length
    info.pv_table.length[info.ply] = info.ply;

    if (depth <= 0)
    {
        return Quiescence(alpha, beta, board, info, ss);
    }

    /* Checking for time every 2048 nodes */
    if ((info.nodes & 2047) == 0)
    {
        CheckUp(info);
    }

    /* We return static evaluation if we exceed max depth */
    if (info.ply > MAXPLY - 1)
    {
        return Evaluate(board);
    }

    /* Repetition check */
    if (board.isRepetition())
    {
        return 0;
    }

    bool isRoot = (info.ply == 0);
    bool inCheck = board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));
    bool isPvNode = (beta - alpha) > 1;
    int score = -INF_BOUND;

    ss->static_eval = inCheck ? 0 : Evaluate(board);

    /* In check extension */
    if (inCheck)
    {
        depth++;
    }

    if (!isPvNode)
    {
        // Null move pruning
        if (!inCheck && info.ply && board.nonPawnMat(board.sideToMove) && (depth >= 3) && ((ss - 1)->move != NULL_MOVE) && ss->static_eval >= beta)
        {
            board.makeNullMove();
            ss->move = NULL_MOVE;
            info.ply++;

            score = -AlphaBeta(-beta, -beta + 1, depth - 2, board, info, ss + 1);

            info.ply--;
            board.unmakeNullMove();

            if (info.stopped)
            {
                return 0;
            }

            if (score >= beta)
            {
                // dont return mate scores
                if (score >= ISMATE)
                    score = beta;

                return score;
            }
        }
    }

    /* Initialize variables */
    int bestscore = -INF_BOUND;
    int MovesSearched = 0;

    // Reset score
    score = -INF_BOUND;

    /* Move generation */
    /* Generate all legal moves for current position*/
    Movelist list;
    Movegen::legalmoves<ALL>(board, list);

    // Score moves
    score_moves(board, &list, ss, &info);

    /* Move loop */
    for (int i = 0; i < list.size; i++)
    {
        // Pick move with highest possible score
        pickNextMove(i, &list);

        // Initialize move variable
        Move move = list.list[i].move;

        bool isQuiet = (!promoted(move) && !is_capture(board, move));

        // Make the move on board
        board.makeMove(move);
        ss->move = move;
        // Increment ply and nodes
        info.ply++;
        info.nodes++;
        MovesSearched++;

        score = -AlphaBeta(-beta, -alpha, depth - 1, board, info, ss + 1);

        // Undo move on board
        board.unmakeMove(move);
        // Decrement ply
        info.ply--;

        if (info.stopped == true && !isRoot)
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

                info.pv_table.array[info.ply][info.ply] = move;
                for (int next_ply = info.ply + 1; next_ply < info.pv_table.length[info.ply + 1]; next_ply++)
                {
                    info.pv_table.array[info.ply][next_ply] = info.pv_table.array[info.ply + 1][next_ply];
                }

                info.pv_table.length[info.ply] = info.pv_table.length[info.ply + 1];

                if (score >= beta)
                {
                    // Update killers
                    if (isQuiet)
                    {
                        ss->killers[1] = ss->killers[0];
                        ss->killers[0] = move;
                    }

                    break;
                }

                info.searchHistory[board.pieceAtB(from(move))][to(move)] += depth;
            }
        }

        // We are safe to break out of the loop if we are sure that we don't return illegal move
        if (info.stopped == true && isRoot && info.pv_table.array[0][0] != NO_MOVE)
        {
            break;
        }
    }

    if (MovesSearched == 0)
    {
        if (inCheck)
        {
            return -ISMATE + info.ply; // Checkmate
        }
        else
        {
            return 0; // Stalemate
        }
    }

    return bestscore;
}

void SearchPosition(Board &board, SearchInfo &info)
{
    ClearForSearch(info);

    // Initialize search stack

    SearchStack stack[MAXPLY + 10];
    SearchStack *ss = stack + 7; // Have some safety overhead.

    int score = 0;

    long startime = GetTimeMs();
    Move bestmove = NO_MOVE;

    for (int current_depth = 1; current_depth <= info.depth; current_depth++)
    {
        score = AlphaBeta(-INF_BOUND, INF_BOUND, current_depth, board, info, ss);
        if (info.stopped == true)
        {
            break;
        }
        bestmove = info.pv_table.array[0][0];
        std::cout << "info score cp " << score << " depth " << current_depth << " nodes " << info.nodes << " time " << (GetTimeMs() - startime) << " pv";

        for (int i = 0; i < info.pv_table.length[0]; i++)
        {
            std::cout << " " << convertMoveToUci(info.pv_table.array[0][i]) << " ";
        }
        std::cout << "\n";
    }

    std::cout << "bestmove " << convertMoveToUci(bestmove) << "\n";
}