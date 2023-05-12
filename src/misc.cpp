#include <chrono>
#include "misc.h"

long GetTimeMs()
{
    std::chrono::milliseconds sec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    return sec.count();
}

std::string san(Board& board, const Move move) {
    // static const std::string repPieceType[] = {"", "N", "B", "R", "Q", "K"};
    // static const std::string repFile[] = {"a", "b", "c", "d", "e", "f", "g", "h"};

    // if (piece(move) == KING && square_distance(to(move), from(move)) >= 2) {
    //     return to(move) > from(move) ? "O-O" : "O-O-O";
    // }

    // const PieceType pt = type_of_piece(board.pieceAtB(from(move)));

    // std::string san;

    // if (pt != PieceType::PAWN) {
    //     san += repPieceType[int(pt)];
    // }

    // Movelist moves;
    // Movegen::legalmoves<ALL>(board, moves);

    // for (const auto &m : moves.list) {
    //     if (pt != PAWN && m != move && board.pieceAtB(from(m)) == board.pieceAtB(from(move)) &&
    //         to(m) == to(move)) {
    //         if (square_file(from(m)) == square_file(from(move))) {
    //             san += std::to_string(int(square_rank(from(move))) + 1);
    //             break;
    //         } else {
    //             san += repFile[int(square_file(from(move)))];
    //             break;
    //         }
    //     }
    // }

    // if (board.pieceAtB(to(move)) != Piece::NONE || move.typeOf() == Move::EN_PASSANT) {
    //     if (pt == PieceType::PAWN) {
    //         san += repFile[int(squareFile(move.from()))];
    //     }

    //     san += "x";
    // }

    // san += repFile[int(squareFile(move.to()))];
    // san += std::to_string(int(squareRank(move.to())) + 1);

    // if (move.typeOf() == Move::PROMOTION) {
    //     san += "=";
    //     san += repPieceType[int(move.promotionType())];
    // }

    // makeMove(move);

    // if (isKingAttacked()) {
    //     if (isGameOver().second == GameResult::LOSE) {
    //         san += "#";
    //     } else {
    //         san += "+";
    //     }
    // }

    // unmakeMove(move);

    // return san;
}