#include "pawns.h"
#include "eval.h"

int DoublePenaltyMg = 1;
int DoublePenaltyEg = 15;
int IsolatedPenaltyMg = 8;
int IsolatedPenaltyEg = 14;

// Pawn Penalties
Score DoublePawnPenalty(-DoublePenaltyMg, -DoublePenaltyEg);
Score IsolatedPenalty(-IsolatedPenaltyMg, -IsolatedPenaltyEg);

void  UpdatePawnTables(){
    DoublePawnPenalty = Score(-DoublePenaltyMg, -DoublePenaltyEg);
    IsolatedPenalty = Score(-IsolatedPenaltyMg, -IsolatedPenaltyEg);
}

static inline bool IsIsolatedPawn(const U64 pawn_bb, Square sq)
{
    return (pawn_bb & IsolatedMasks[sq]) == 0;
}

static inline uint8_t CountDoublePawns(const U64 pawn_bb, Square sq)
{
    return popcount(pawn_bb & FileMasks[sq]);
}

static Score EvaluatePawns(U64 pawns_bb)
{
    Score score;
    
    while (pawns_bb)
    {
        Square sq = poplsb(pawns_bb);
        if (IsIsolatedPawn(pawns_bb, sq))
        {
            score += IsolatedPenalty;
            
        }
        // if (CountDoublePawns(pawns_bb, sq) > 1){
        //     score += DoublePawnPenalty;
        // }
    }
    return score;
}

PawnTable::PawnTable()
{
    PawnEntry entry = PawnEntry();
    PawnEntry *e = &entry;
    this->entries.resize((512*1024) / sizeof(PawnEntry), e);
    std::fill(entries.begin(), entries.end(), e);
}

PawnEntry *PawnTable::probeEntry(const Board &board)
{
    PawnEntry *pe = entries[board.pawnKey % entries.size()];

    if (pe->hashkey == board.pawnKey) {
        return (pe);
    }

    pe->hashkey = board.hashKey;

    pe->value = Score(0,0);

    U64 white_pawns = board.piecesBB[WhitePawn];
    U64 black_pawns = board.piecesBB[BlackPawn];

    // pe->attacks[White] = Movegen::pawnLeftAttacks<White>(white_pawns);
    // pe->attacks2[White] = Movegen::pawnRightAttacks<White>(white_pawns);

    // pe->attacks[Black] = Movegen::pawnLeftAttacks<Black>(black_pawns);
    // pe->attacks2[Black] = Movegen::pawnRightAttacks<Black>(black_pawns);

    pe->value += EvaluatePawns(white_pawns);
    pe->value -= EvaluatePawns(black_pawns);

    return (pe);
}

void PawnTable::clear(){
    entries.clear();
}