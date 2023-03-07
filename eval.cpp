#include "eval.h"

// Piece square table for Pawns
const int pawn_table[64] = {
 0,  0,  0,  0,  0,  0,  0,  0,
50, 50, 50, 50, 50, 50, 50, 50,
10, 10, 20, 30, 30, 20, 10, 10,
 5,  5, 10, 25, 25, 10,  5,  5,
 0,  0,  0, 20, 20,  0,  0,  0,
 5, -5,-10,  0,  0,-10, -5,  5,
 5, 10, 10,-20,-20, 10, 10,  5,
 0,  0,  0,  0,  0,  0,  0,  0
};

// Piece square table for Knights
const int knight_table[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
-40,-20,  0,  0,  0,  0,-20,-40,
-30,  0, 10, 15, 15, 10,  0,-30,
-30,  5, 15, 20, 20, 15,  5,-30,
-30,  0, 15, 20, 20, 15,  0,-30,
-30,  5, 10, 15, 15, 10,  5,-30,
-40,-20,  0,  5,  5,  0,-20,-40,
-50,-40,-30,-30,-30,-30,-40,-50,
};

// Piece square table for Bishops
const int bishop_table[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
-10,  0,  0,  0,  0,  0,  0,-10,
-10,  0,  5, 10, 10,  5,  0,-10,
-10,  5,  5, 10, 10,  5,  5,-10,
-10,  0, 10, 10, 10, 10,  0,-10,
-10, 10, 10, 10, 10, 10, 10,-10,
-10,  5,  0,  0,  0,  0,  5,-10,
-20,-10,-10,-10,-10,-10,-10,-20,
};

// Piece square table for Rooks
const int rook_table[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
  5, 10, 10, 10, 10, 10, 10,  5,
 -5,  0,  0,  0,  0,  0,  0, -5,
 -5,  0,  0,  0,  0,  0,  0, -5,
 -5,  0,  0,  0,  0,  0,  0, -5,
 -5,  0,  0,  0,  0,  0,  0, -5,
 -5,  0,  0,  0,  0,  0,  0, -5,
  0,  0,  0,  5,  5,  0,  0,  0
};

// Piece square table for Queens
const int queen_table[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
-10,  0,  0,  0,  0,  0,  0,-10,
-10,  0,  5,  5,  5,  5,  0,-10,
 -5,  0,  5,  5,  5,  5,  0, -5,
  0,  0,  5,  5,  5,  5,  0, -5,
-10,  5,  5,  5,  5,  5,  0,-10,
-10,  0,  5,  0,  0,  0,  0,-10,
-20,-10,-10, -5, -5,-10,-10,-20
};

// Piece square table for Kings
const int king_table[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
-30,-40,-40,-50,-50,-40,-40,-30,
-30,-40,-40,-50,-50,-40,-40,-30,
-30,-40,-40,-50,-50,-40,-40,-30,
-20,-30,-30,-40,-40,-30,-30,-20,
-10,-20,-20,-20,-20,-20,-20,-10,
 20, 20,  0,  0,  0,  0, 20, 20,
 20, 30, 10,  0,  0, 10, 30, 20
};

const int piece_values[12] = {100, 320, 330, 500, 900, 0};

int Evaluate(Board& board){
    int score = 0;
    int material = 0;

    U64 white_pieces = board.Us(White);
    U64 black_pieces = board.Us(Black);

    // Evaluate White's Pieces
    while (white_pieces){
        Square sq = poplsb(white_pieces);
        Piece p = board.pieceAtB(sq);

        // Add material score
        material += piece_values[PieceToPieceType[p]];
        switch (p){

            // Evaluate for pawns

            case WhitePawn:
            score += pawn_table[sq^56]; // We mirror square if it's white because bitboards have pieces flipped.
            break;

            // Evaluate for knights
            case WhiteKnight:
            score += knight_table[sq^56];
            break;

            // Evaluate for bishops
            case WhiteBishop:
            score += bishop_table[sq^56];
            break;

            // Evaluate for rooks
            case WhiteRook:
            score += rook_table[sq^56];
            break;

            // Evaluate for queens
            case WhiteQueen:
            score += queen_table[sq^56];
            break;

            // Evaluate for kings
            case WhiteKing:
            score += king_table[sq^56];
            break;
            default:
            break;
        }
    }

    // Evaluate Black's pieces
    while (black_pieces){
        Square sq = poplsb(black_pieces);
        Piece p = board.pieceAtB(sq);
        
        material -= piece_values[PieceToPieceType[p]];
        
        switch (p){

            // Evaluate for pawns

            case BlackPawn:
            score -= pawn_table[sq];
            break;

            // Evaluate for knights
            case BlackKnight:
            score -= knight_table[sq];
            break;

            // Evaluate for bishops
            case BlackBishop:
            score -= bishop_table[sq];
            break;

            // Evaluate for rooks
            case BlackRook:
            score -= rook_table[sq];
            break;

            // Evaluate for queens
            case BlackQueen:
            score -= queen_table[sq];
            break;

            // Evaluate for kings
            case BlackKing:
            score -= king_table[sq];
            break;
            default:
            break;
        }
    }

    score += material;

    /* We make the score negative if the side to move is black. */
    /* This is required in a Negamax framework. */
    if (board.sideToMove == Black){
        return -score;
    }

    return score;
}