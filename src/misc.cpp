#include <chrono>
#include "misc.h"

long GetTimeMs()
{
    std::chrono::milliseconds sec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    return sec.count();
}

std::string getSanString(Board &board, Move move)
{
    std::stringstream ss;

    bool inCheck = false;
    board.makeMove(move);

    if (board.isSquareAttacked(~board.sideToMove, board.KingSQ(board.sideToMove)))
    {
        inCheck = true;
    }

    board.unmakeMove(move);

    // Get the from and to squares
    Square from_sq = from(move);
    Square to_sq = to(move);
    Piece p = board.pieceAtB(from_sq);
    PieceType pt = type_of_piece(p);
    bool capture = is_capture(board, move);

    if (piece(move) == KING && square_distance(to_sq, from_sq) >= 2)
    {
        ss << (to_sq > from_sq ? "O-O" : "O-O-O");
    }

    // Add the from and to squares to the string stream
    if (pt != PAWN)
    {
        ss << pieceToChar[Piece(pt)];
    }
    else
    {
        ss << char('a' + square_file(from_sq));
    }

    if (capture)
    {
        ss << "x";  
    }

    if (pt != PAWN || capture)
    {
        ss << char('a' + square_file(to_sq));
    }

    ss << char('1' + square_rank(to_sq));

    // If the move is a promotion, add the promoted piece to the string stream
    if (promoted(move))
    {
        ss << "=";
        ss << PieceTypeToPromPiece[piece(move)];
    }
    if (inCheck)
    {
        ss << "+";
    }

    return ss.str();
}
