#include "pawns.h"
#include "eval.h"

// Pawn Penalties
Score PASSED_PAWN_BONUS(1, 19);
Score BLOCKED_PASSED_PAWN_PENALTY(5, -20);
Score ISOLATED_PAWN_PENALTY(-24, -13);
Score DOUBLE_PAWN_PENATLY(-13, -32);

static inline bool IsIsolatedPawn(const U64 pawn_bb, Square sq)
{
    return (pawn_bb & IsolatedMasks[sq]) == 0;
}

static inline uint8_t CountDoublePawns(const U64 pawn_bb, Square sq)
{
    return (popcount(pawn_bb & FileMasks[sq]));
}

template <Color c>
Score EvaluatePawns(U64 pawns_bb, U64 enemy, U64 all)
{
    Score score;
    Square sq;
    Rank rank;
    while (pawns_bb)
    {
        sq = poplsb(pawns_bb);

        if (IsIsolatedPawn(pawns_bb, sq))
        {
            score += ISOLATED_PAWN_PENALTY;
        }
        if (c == White)
        {
            if ((WhitePassedMasks[sq] & ~enemy) == 0)
            {
                rank = square_rank(sq);

                score += PASSED_PAWN_BONUS * (7 - rank);
            }
            if ((WhitePassedMasks[sq] & ~all))
            {
                score += BLOCKED_PASSED_PAWN_PENALTY;
            }
        }
        else
        {
            if ((BlackPassedMasks[sq] & ~enemy) == 0)
            {
                rank = square_rank(sq);
                score += PASSED_PAWN_BONUS * rank;
            }
            if ((BlackPassedMasks[sq] & ~all))
            {
                score += BLOCKED_PASSED_PAWN_PENALTY;
            }
        }
        //score += DOUBLE_PAWN_PENATLY * (CountDoublePawns(pawns_bb, sq) - 1);
    }
    return score;
}

PawnTable::PawnTable()
{
    PawnEntry entry = PawnEntry();
    this->entries.resize((512 * 1024) / sizeof(PawnEntry), &entry);
    std::fill(entries.begin(), entries.end(), &entry);
}

void PawnTable::Reinitialize(){
     PawnEntry entry = PawnEntry();
    this->entries.resize((512 * 1024) / sizeof(PawnEntry), &entry);
    std::fill(entries.begin(), entries.end(), &entry);
}

PawnEntry *PawnTable::probeEntry(const Board &board)
{
    PawnEntry *pe = (entries[board.pawnKey % entries.size()]);

    if (pe->hashkey == board.pawnKey)
    {
        return (pe);
    }

    pe->hashkey = board.pawnKey;

    pe->value = Score(0, 0);

    U64 white_pawns = board.piecesBB[WhitePawn];
    U64 black_pawns = board.piecesBB[BlackPawn];

    // pe->attacks[White] = Movegen::pawnLeftAttacks<White>(white_pawns);
    // pe->attacks2[White] = Movegen::pawnRightAttacks<White>(white_pawns);

    // pe->attacks[Black] = Movegen::pawnLeftAttacks<Black>(black_pawns);
    // pe->attacks2[Black] = Movegen::pawnRightAttacks<Black>(black_pawns);

    pe->value += EvaluatePawns<White>(white_pawns, black_pawns, board.All());
    pe->value -= EvaluatePawns<Black>(black_pawns, white_pawns, board.All());

    return (pe);
}

void PawnTable::clear()
{
    entries.clear();
}