#include "movescore.h"
#include "eval.h"

int mvv_lva[12][12] = {
    105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600,

    105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600};

// Move scoring
void score_moves(Board &board, Movelist *list, SearchStack *ss, SearchInfo *info)
{

    // Loop through moves in movelist.
    for (int i = 0; i < list->size;i++)
    {
        Piece victim = board.pieceAtB(to(list->list[i].move));
        Piece attacker = board.pieceAtB(from(list->list[i].move));
        if (victim != None)
        {
            // If it's a capture move, we score using MVVLVA (Most valuable victim, Least Valuable Attacker)
            // Indexed by [victim][attacker]
            list->list[i].value = mvv_lva[attacker][victim] + 10000000;
        }else if (list->list[i].move == ss->killers[0]){
            // Score for killer 1
            list->list[i].value = 9000000;
        }else if (list->list[i].move == ss->killers[1]){
            // Score for killer 2
            list->list[i].value = 8000000;
        }else{
            list->list[i].value = info->searchHistory[attacker][to(list->list[i].move)];
        }
    }
}

// This is used to score only mvv lva.
void score_moves(Board &board, Movelist *list)
{

    // Loop through moves in movelist.
    for (int i = 0; i < list->size;i++)
    {
        Piece victim = board.pieceAtB(to(list->list[i].move));
        Piece attacker = board.pieceAtB(from(list->list[i].move));
        if (victim != None)
        {
            // If it's a capture move, we score using MVVLVA (Most valuable victim, Least Valuable Attacker)
            list->list[i].value = mvv_lva[attacker][victim] + 10000000;
        }
    }
}

void pickNextMove(const int moveNum, Movelist *list)
{

    ExtMove temp;
    int index = 0;
    int bestscore = -INF_BOUND;
    int bestnum = moveNum;

    for (index = moveNum; index < list->size; ++index)
    {

        if (list->list[index].value > bestscore)
        {
            bestscore = list->list[index].value;
            bestnum = index;
        }
    }

    temp = list->list[moveNum];
    list->list[moveNum] = list->list[bestnum];
    list->list[bestnum] = temp;
}