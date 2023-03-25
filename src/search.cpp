#include "search.h"
#include "eval.h"
#include "misc.h"
#include "movescore.h"
#include "see.h"
#include <cmath>
#include <iostream>
#include "fancyterminal.h"

/* Refer to init.cpp for search parameter values. */


int LMRTable[MAXDEPTH][64];

/* Initialize LMR table using the log formula */
void InitSearch() {
  float b = static_cast<float>(static_cast<float>(LMRBase)/100);
  float d = static_cast<float>(static_cast<float>(LMRDivision)/100);
  for (int depth = 1; depth < MAXDEPTH; depth++) {
    for (int played = 1; played < 64; played++) {
      LMRTable[depth][played] = b + log(depth) * log(played) / d;
    }
  }
}


/* Function to check if time has ended and if we have to stop. */
static void CheckUp(SearchInfo &info) {
  if (info.timeset == true && GetTimeMs() > info.end_time) {
    info.stopped = true;
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

  /* Increment transposition table age */
  table->currentAge++;
}

/* Quiescence Search to prevent Horizon Effect.*/
int Quiescence(int alpha, int beta, Board &board, SearchInfo &info,
               SearchStack *ss, TranspositionTable *table) {

  /* Checking for time every 2048 nodes */
  if ((info.nodes & 2047) == 0) {
    CheckUp(info);
  }

  /* We return static evaluation if we exceed max depth */
  if (info.ply > MAXPLY - 1) {
    return Evaluate(board, info.pawnTable);
  }

  /* Repetition check */
  if (board.isRepetition()) {
    return 0;
  }

  /* standing_pat is our static evaluation of the board */
  int standing_pat = Evaluate(board, info.pawnTable);

  /* if our static evaluation beats beta, we return beta.*/
  if (standing_pat >= beta) {
    return beta;
  }

  /* if our static evaluation betas alpha, we set alpha to standing_pat */
  if (standing_pat > alpha) {
    alpha = standing_pat;
  }

  /* Probe Tranpsosition Table */
  TTEntry tte;
  bool isPvNode = (beta - alpha) > 1;
  bool ttHit = table->probeEntry(board.hashKey, &tte, info.ply);

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
    
    /* Increment ply, nodes and movecount */
    info.ply++;
    info.nodes++;
    moveCount++;

    ss->move = move;

    /* Recursive call of Quiescence on current position */
    score = -Quiescence(-beta, -alpha, board, info, ss + 1, table);

    /* Undo move on board */
    board.unmakeMove(move);

    /* Decrement ply */
    info.ply--;

    /* Return 0 if time is up */
    if (info.stopped) {
      return 0;
    }

    /* If our score beats bestscore, bestmove is move that beat bestscore and bestscore is set to the current score*/
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
  /* We don't store exact flag in quiescence search, only Beta flag (Lower Bound) and Alpha flag (Upper bound)*/
  int flag = bestscore >= beta ? HFBETA : HFALPHA;

  /* Store transposition table entry */
  table->storeEntry(board.hashKey, flag, bestmove, 0, bestscore, standing_pat,
                    info.ply, isPvNode);

  /* Return bestscore achieved */
  return bestscore;
}

/* Function based on the Negamax framework and alpha-beta pruning */
/* This is our main Search function , which we use to find "Good moves". */
int AlphaBeta(int alpha, int beta, int depth, Board &board, SearchInfo &info,
              SearchStack *ss, TranspositionTable *table) {

  /* Initialize our pv table lenght. */
  info.pv_table.length[info.ply] = info.ply;

  /* We drop into quiescence search if depth is <= 0 to prevent horizon effect and also end recursion.*/
  if (depth <= 0) {
    return Quiescence(alpha, beta, board, info, ss, table);
  }

  /* Checking for time every 2048 nodes */
  if ((info.nodes & 2047) == 0) {
    CheckUp(info);
  }

  /* Initialize helper variables */
  bool isRoot = (info.ply == 0);
  bool inCheck =
      board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));
  bool isPvNode = (beta - alpha) > 1;
  int score = -INF_BOUND;
  int eval = 0;
  bool improving = false;

  /* In check extension. We search an extra ply if we are in check. */
  if (inCheck){
    depth++;
  }

  /* We return static evaluation if we exceed max depth.*/
  if (info.ply > MAXPLY - 1) {
    return Evaluate(board, info.pawnTable);
  }

  /* Repetition check*/
  if ((board.isRepetition()) && info.ply) {
    return 0;
  }

  /* Probe transposition table */
  TTEntry tte;
  bool ttHit = ss->excluded != NO_MOVE
                   ? false
                   : table->probeEntry(board.hashKey, &tte, info.ply);

  /* If we hit a transposition table entry and the depth >= tt entry depth
     and the following conditions meet we will return the tt score. */

  if (!isPvNode && ttHit && tte.depth >= depth) {
    if ((tte.flag == HFALPHA && tte.score <= alpha) ||
        (tte.flag == HFBETA && tte.score >= beta) || (tte.flag == HFEXACT))
      return tte.score;
  }

  /* Set static evaluation and evaluation to our current evaluation of the board*/
  /* We can use the tt entry's evaluation if we have a tt hit so we don't have to re-evaluate from scratch */

  ss->static_eval = eval = ttHit ? tte.eval : Evaluate(board, info.pawnTable);

  /* If we our static evaluation is better than what it was 2 plies ago, we are improving */
  improving = !inCheck && ss->static_eval > (ss - 2)->static_eval;

  /* We set static evaluation to 0 if we are in check or if we have hit an excluded move. */
  /* and also set improving to false. */
  if (inCheck || ss->excluded) {
    ss->static_eval = eval = 0;
    improving = false;
    goto movesloop;
  }

  /* Reverse Futility Pruning || Null Move Pruning */
  if (!isPvNode && !inCheck && info.ply && !ss->excluded) {

    /* We can use tt entry's score as score if we hit one.*/
    /* This score is from search so this is more accurate */
    if (ttHit) {
      eval = tte.score;
    }

    /* Reverse Futility Pruning (RFP)
     * If the eval is well above beta by a margin, then we assume the eval will
     * hold above beta.
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
        ((ss - 1)->move != NULL_MOVE)) {

      int R = 3 + depth / 3 + std::min((eval - beta) / 200, 3);
      
      board.makeNullMove();
      ss->move = NULL_MOVE;
      info.ply++;

      score =
          -AlphaBeta(-beta, -beta + 1, depth - R, board, info, ss + 1, table);

      info.ply--;
      board.unmakeNullMove();

      if (info.stopped) {
        return 0;
      }

      if (score >= beta) {
        
        /* We don't return mate scores because it can be a false mate. */
        if (score > ISMATE)
          score = beta;

        return score;
      }
    }    
  }

  movesloop:

  /* Initialize variables */
  int bestscore = -INF_BOUND;
  int moveCount = 0;
  int quietsSearched = 0;
  int oldAlpha = alpha;
  Move bestmove = NO_MOVE;

  /* Reset score back to infinite since we changed it in null move pruning*/
  score = -INF_BOUND;

  /* Move generation */
  /* Generate all legal moves for current position */
  Movelist list;
  Movegen::legalmoves<ALL>(board, list);

  /* Score moves and pass the tt move so it can be sorted highest */
  score_moves(board, list, ss, info, tte.move);

  /* Move loop */
  for (int i = 0; i < list.size; i++) {
    // Pick move with highest possible score
    pickNextMove(i, list);

    /* Initialize move variable */
    Move move = list.list[i].move;

    /* Skip excluded moves from extension */
    if (move == ss->excluded)
      continue;

    bool isQuiet = (!promoted(move) && !is_capture(board, move));
    bool skipQuiets = false;
    int extension = 0;

    if (isQuiet && skipQuiets){
      continue;
    }

    if (!isRoot && bestscore > -ISMATE) {

      /* Various pruning techniques */
      if (isQuiet) {
        /* Late Move Pruning/Movecount pruning */
        /* If we have searched many moves, we can skip the rest. */
        if (!isPvNode && !inCheck && depth <= 4 &&
            (quietsSearched >= depth * depth * 4)) {
          continue;
        }

        /* SEE Pruning
        * if we don't SEE a good move by a given threshold, and we are under a certain depth, we can skip the moves.
        */

        /* SEE Pruning for quiets */
        if (depth < 6 && !see(board, move, -50 * depth)) {
          continue;
        }

      } else {

        /* SEE Pruning for non quiets */
        if (depth < 4 && !see(board, move, -45 * depth)) {
          continue;
        }
      }
    }

    

    /* Initialize new depth based on extension*/
    int newDepth = depth + extension;

    /* Make move on current board. */
    board.makeMove(move);

    ss->move = move;
    
    /* Increment ply, nodes and movecount */
    info.ply++;
    info.nodes++;
    moveCount++;

    /* Increment quietsSearched variable if it's a quiet move. */
    if (isQuiet) {
      quietsSearched++;
    }

    /* A condition for full search.*/
    bool do_fullsearch = false;
    
    bool givesCheck = board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove));

    /* Late move reduction
     * Later moves will be searched in a reduced depth.
     * If they beat alpha, It will be researched in a reduced window but full depth.
     */
    if (!inCheck && depth >= 3 && moveCount > (2 + 2 * isPvNode))  {
      int reduction = LMRTable[std::min(depth, 63)][std::min(63, moveCount)];

      reduction += !improving; /* Increase reduction if we're not improving. */
      reduction += !isPvNode; /* Increase for non pv nodes */
      reduction += isQuiet && !see(board, move, -50*depth); // Increase for quiets and not winning captures

      /* Adjust the reduction so we don't drop into Qsearch or cause an extension*/
      reduction = std::min(depth - 1, std::max(1, reduction));

      score = -AlphaBeta(-alpha - 1, -alpha, newDepth - reduction, board, info,
                         ss + 1, table);

      /* We do a full depth research if our score beats alpha. */
      do_fullsearch = score > alpha && reduction != 1;
    } else {

      /* Zero window search. */
      do_fullsearch = !isPvNode || moveCount > 1;
    }

    /* Full depth search on a zero window. */
    if (do_fullsearch) {
      score = -AlphaBeta(-alpha - 1, -alpha, newDepth - 1, board, info, ss + 1,
                         table);
    }

    // Principal Variation Search (PVS)
    if (isPvNode && (moveCount == 1 || (score > alpha && score < beta))) {
      score =
          -AlphaBeta(-beta, -alpha, newDepth - 1, board, info, ss + 1, table);
    }

    // Undo move on board
    board.unmakeMove(move);
    // Decrement ply
    info.ply--;

    if (info.stopped == true && !isRoot) {
      return 0;
    }

    if (score > bestscore) {
      bestscore = score;

      if (score > alpha) {
        // Record PV
        alpha = score;

        bestmove = move;

        info.pv_table.array[info.ply][info.ply] = bestmove;
        for (int next_ply = info.ply + 1;
             next_ply < info.pv_table.length[info.ply + 1]; next_ply++) {
          info.pv_table.array[info.ply][next_ply] =
              info.pv_table.array[info.ply + 1][next_ply];
        }

        info.pv_table.length[info.ply] = info.pv_table.length[info.ply + 1];

        if (score >= beta) {
          // Update killers
          if (isQuiet) {

            ss->killers[1] = ss->killers[0];
            ss->killers[0] = move;

            // Record history score
            info.searchHistory[board.pieceAtB(from(move))][to(move)] += depth;
          }
          break;
        }
      }
    }

    // We are safe to break out of the loop if we are sure that we don't return
    // illegal move
    if (info.stopped == true && isRoot &&
        info.pv_table.array[0][0] != NO_MOVE) {
      break;
    }
  }

  if (moveCount == 0) {
    if (inCheck) {
      return -ISMATE + info.ply; // Checkmate
    } else {
      return 0; // Stalemate
    }
  }

  int flag = bestscore >= beta     ? HFBETA
             : (alpha != oldAlpha) ? HFEXACT
                                   : HFALPHA;

  if (ss->excluded == NO_MOVE) {
    table->storeEntry(board.hashKey, flag, bestmove, depth, bestscore,
                      ss->static_eval, info.ply, isPvNode);
  }

  return bestscore;
}

void SearchPosition(Board &board, SearchInfo &info, TranspositionTable *table) {
  ClearForSearch(info, table);

  // Initialize search stack

  int score = 0;

  long startime = GetTimeMs();
  Move bestmove = NO_MOVE;

  for (int current_depth = 1; current_depth <= info.depth; current_depth++) {
    score = AspirationWindowSearch(
        score, current_depth, board, info,
        table); // AlphaBeta(-INF_BOUND, INF_BOUND, current_depth, board, info,
                // ss, table);
    if (info.stopped == true) {
      break;
    }
    
    bestmove = info.pv_table.array[0][0];
    std::cout << "info score cp ";
    F_number(score, info.uci, FANCY_Yellow);
    std::cout << " depth ";
    F_number(current_depth,info.uci, FANCY_Green);
    std::cout << " nodes "; 
    F_number(info.nodes,info.uci, FANCY_Yellow);
    std::cout<< " time ";
    F_number((GetTimeMs() - startime),info.uci, FANCY_Cyan);
    std::cout << "pv";

    for (int i = 0; i < info.pv_table.length[0]; i++) {
      std::cout << " " << convertMoveToUci(info.pv_table.array[0][i]);
    }
    std::cout << "\n";
  }

  std::cout << "bestmove " << convertMoveToUci(bestmove) << "\n";
}

int AspirationWindowSearch(int prevEval, int depth, Board &board,
                           SearchInfo &info, TranspositionTable *table) {
  int score = 0;

  SearchStack stack[MAXPLY + 10];
  SearchStack *ss = stack + 7; // Have some safety overhead.

  int delta = 12;

  int alpha = -INF_BOUND;
  int beta = INF_BOUND;

  if (depth > 3) {
    alpha = std::max(-INF_BOUND, prevEval - delta);
    beta = std::min(prevEval + delta, INF_BOUND);
  }

  while (true) {
    score = AlphaBeta(alpha, beta, depth, board, info, ss, table);

    CheckUp(info);
    if (info.stopped) {
      break;
    }

    if ((score <= alpha)) {
      beta = (alpha + beta) / 2;
      alpha = std::max(-INF_BOUND, score - delta);
    } else if ((score >= beta)) {
      beta = std::min(score + delta, INF_BOUND);
    } else {
      break;
    }
    delta += delta / 2;
  }
  return score;
}