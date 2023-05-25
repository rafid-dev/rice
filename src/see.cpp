#include "see.h"

const int pieceValues[7] = { 93, 308, 346, 521, 994, 20000, 0};

bool see(Board &board, const Move move, const int threshold)
{
    Square to_square = to(move);
    Square from_square = from(move);

    PieceType target = type_of_piece(board.pieceAtB(to_square));

    int value = pieceValues[target] - threshold;

    if (value < 0)
    {
        return false;
    }

    PieceType attacker = type_of_piece(board.pieceAtB(from_square));

    value -= pieceValues[attacker];

    if (value >= 0)
    {
        return true;
    }

    U64 occupied = (board.All() ^ (1ULL << from_square)) | (1ULL << to_square);
    U64 attackers = board.allAttackers(to_square, occupied) & occupied;

    U64 queens = board.piecesBB[WhiteQueen] | board.piecesBB[BlackQueen];

    U64 bishops = board.piecesBB[WhiteBishop] | board.piecesBB[BlackBishop] | queens;
    U64 rooks = board.piecesBB[WhiteRook] | board.piecesBB[BlackRook] | queens;

    Color st = ~board.colorOf(from_square);

    while (true){
        attackers &= occupied;

        U64 myAttackers = attackers & board.Us(st);

        if (!myAttackers){
            break;
        }

        int pt;
        for (pt = 0; pt <= 5;pt++){
            if (myAttackers & (board.piecesBB[pt] | board.piecesBB[pt+6])){
                break;
            }
        }

        st = ~st;
        if ((value = -value - 1 - pieceValues[pt]) >= 0)
        {
            if (pt == KING && (attackers & board.Us(st)))
                st = ~st;
            break;
        }

        occupied ^= (1ULL << (lsb(myAttackers & (board.piecesBB[pt] | board.piecesBB[pt + 6]))));

        if (pt == PAWN || pt == BISHOP || pt == QUEEN)
            attackers |= BishopAttacks(to_square, occupied) & bishops;
        if (pt == ROOK || pt == QUEEN)
            attackers |= RookAttacks(to_square, occupied) & rooks;
    }
    return st != board.colorOf(from_square);
}