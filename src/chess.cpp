#include "types.h"
#define INCBIN_STYLE INCBIN_STYLE_CAMEL
#include "incbin/incbin.h"

INCBIN(EVAL, "./sn.nnue");

void Board::Refresh() {
    nnue->ResetAccumulator();
    nnue->RefreshAccumulator();

    for (Square sq = SQ_A1; sq < NO_SQ; sq++) {
        PieceType p = pieceTypeAtB(sq);

        if (p == NONETYPE)
            continue;

        nnue->EfficientlyUpdateAccumulator<
            MantaRay::AccumulatorOperation::Activate>(p, colorOf(sq), sq);
    }
}

Board::Board(std::string fen) {
    initializeLookupTables();
    stateHistory.reserve(MAX_PLY);
    hashHistory.reserve(512);
    pawnKeyHistory.reserve(512);

    MantaRay::BinaryMemoryStream stream(gEVALData, gEVALSize);
    // MantaRay::MarlinflowStream stream(R"(./12.json)");

    nnue = new PerspectiveNetwork(stream);
    // MantaRay::BinaryFileStream str(R"(./12.nnue)");
    // nnue->WriteTo(str);

    sideToMove = White;
    enPassantSquare = NO_SQ;
    castlingRights = wk | wq | bk | bq;
    halfMoveClock = 0;
    fullMoveNumber = 1;

    pinHV = 0;
    pinD = 0;
    doubleCheck = 0;
    checkMask = DEFAULT_CHECKMASK;
    seen = 0;

    std::fill(std::begin(board), std::end(board), None);

    applyFen(fen);

    occEnemy = Enemy(sideToMove);
    occUs = Us(sideToMove);
    occAll = All();
    enemyEmptyBB = EnemyEmpty(sideToMove);
}

void Board::makeMove(Move move) {
    PieceType pt = piece(move);
    Piece p = makePiece(pt, sideToMove);
    Square from_sq = from(move);
    Square to_sq = to(move);
    Piece capture = board[to_sq];

    assert(from_sq >= 0 && from_sq < MAX_SQ);
    assert(to_sq >= 0 && to_sq < MAX_SQ);
    assert(type_of_piece(capture) != KING);
    assert(p != None);
    assert((promoted(move) && (pt != PAWN && pt != KING)) || !promoted(move));

    // *****************************
    // STORE STATE HISTORY
    // *****************************

    hashHistory.emplace_back(hashKey);
    stateHistory.emplace_back(
        State(enPassantSquare, castlingRights, halfMoveClock, capture));
    pawnKeyHistory.emplace_back(pawnKey);
    nnue->PushAccumulator();

    halfMoveClock++;
    fullMoveNumber++;

    bool ep = to_sq == enPassantSquare;
    const bool isCastling = pt == KING && type_of_piece(capture) == ROOK &&
                            colorOf(from_sq) == colorOf(to_sq);

    // *****************************
    // UPDATE HASH
    // *****************************

    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);
    enPassantSquare = NO_SQ;

    hashKey ^= updateKeyCastling();

    if (isCastling) {
        Piece rook = sideToMove == White ? WhiteRook : BlackRook;
        Square rookSQ = file_rank_square(to_sq > from_sq ? FILE_F : FILE_D,
                                         square_rank(from_sq));

        assert(type_of_piece(pieceAtB(to_sq)) == ROOK);
        hashKey ^= updateKeyPiece(rook, to_sq);
        hashKey ^= updateKeyPiece(rook, rookSQ);
    }

    if (pt == KING) {
        removeCastlingRightsAll(sideToMove);
    } else if (pt == ROOK) {
        removeCastlingRightsRook(from_sq);
    } else if (pt == PAWN) {
        halfMoveClock = 0;
        if (ep) {
            hashKey ^=
                updateKeyPiece(makePiece(PAWN, ~sideToMove), Square(to_sq ^ 8));
        } else if (std::abs(from_sq - to_sq) == 16) {
            U64 epMask = PawnAttacks(Square(to_sq ^ 8), sideToMove);
            if (epMask & pieces(PAWN, ~sideToMove)) {
                enPassantSquare = Square(to_sq ^ 8);
                hashKey ^= updateKeyEnPassant(enPassantSquare);

                assert(pieceAtB(enPassantSquare) == None);
            }
        }
    }

    if (capture != None && !isCastling) {
        halfMoveClock = 0;
        hashKey ^= updateKeyPiece(capture, to_sq);
        if (type_of_piece(capture) == ROOK)
            removeCastlingRightsRook(to_sq);
    }

    if (promoted(move)) {
        halfMoveClock = 0;

        hashKey ^= updateKeyPiece(makePiece(PAWN, sideToMove), from_sq);
        hashKey ^= updateKeyPiece(p, to_sq);

        if (pt == PAWN) {
            pawnKey ^= updateKeyPiece(makePiece(PAWN, sideToMove), from_sq);
            pawnKey ^= updateKeyPiece(p, to_sq);
        }
    } else {
        hashKey ^= updateKeyPiece(p, from_sq);
        hashKey ^= updateKeyPiece(p, to_sq);
        if (pt == PAWN) {
            pawnKey ^= updateKeyPiece(p, from_sq);
            pawnKey ^= updateKeyPiece(p, to_sq);
        }
    }

    hashKey ^= updateKeySideToMove();
    hashKey ^= updateKeyCastling();

    // *****************************
    // UPDATE PIECES
    // *****************************

    if (isCastling) {
        const Piece rook = sideToMove == White ? WhiteRook : BlackRook;
        Square rookToSq = file_rank_square(to_sq > from_sq ? FILE_F : FILE_D,
                                           square_rank(from_sq));
        Square kingToSq = file_rank_square(to_sq > from_sq ? FILE_G : FILE_C,
                                           square_rank(from_sq));

        // Update accumulator efficiently
        nnue->EfficientlyUpdateAccumulator(ROOK, sideToMove, to_sq, rookToSq);
        nnue->EfficientlyUpdateAccumulator(KING, sideToMove, from_sq, kingToSq);

        removePiece(p, from_sq);
        removePiece(rook, to_sq);

        placePiece(p, kingToSq);
        placePiece(rook, rookToSq);
    } else if (pt == PAWN && ep) {
        assert(pieceAtB(Square(to_sq ^ 8)) != None);

        removePiece(makePiece(PAWN, ~sideToMove), Square(to_sq ^ 8));

        nnue->EfficientlyUpdateAccumulator<
            MantaRay::AccumulatorOperation::Deactivate>(PAWN, ~sideToMove,
                                                        Square(to_sq ^ 8));
    } else if (capture != None && !isCastling) {
        assert(pieceAtB(to_sq) != None);

        if (capture == WhitePawn || capture == BlackPawn) {
            pawnKey ^= updateKeyPiece(capture, to_sq);
        }

        removePiece(capture, to_sq);

        // Remove the captured piece from target square
        nnue->EfficientlyUpdateAccumulator<
            MantaRay::AccumulatorOperation::Deactivate>(type_of_piece(capture),
                                                        ~sideToMove, to_sq);
    }

    if (promoted(move)) {
        assert(pieceAtB(to_sq) == None);

        removePiece(makePiece(PAWN, sideToMove), from_sq);
        placePiece(p, to_sq);

        // Efficiently update the accumulator
        // Remove the pawn from the square and place the promoted piece.
        nnue->EfficientlyUpdateAccumulator<
            MantaRay::AccumulatorOperation::Deactivate>(PAWN, sideToMove,
                                                        from_sq);
        nnue->EfficientlyUpdateAccumulator<
            MantaRay::AccumulatorOperation::Activate>(type_of_piece(p),
                                                      sideToMove, to_sq);
    } else if (!isCastling) {
        assert(pieceAtB(to_sq) == None);

        movePiece(p, from_sq, to_sq);
        nnue->EfficientlyUpdateAccumulator(type_of_piece(p), sideToMove,
                                           from_sq, to_sq);
    }

    sideToMove = ~sideToMove;
}

void Board::unmakeMove(Move move) {
    const State restore = stateHistory.back();
    stateHistory.pop_back();

    hashKey = hashHistory.back();
    hashHistory.pop_back();
    pawnKey = pawnKeyHistory.back();
    pawnKeyHistory.pop_back();

    nnue->PullAccumulator();

    enPassantSquare = restore.enPassant;
    castlingRights = restore.castling;
    halfMoveClock = restore.halfMove;
    Piece capture = restore.capturedPiece;

    fullMoveNumber--;

    Square from_sq = from(move);
    Square to_sq = to(move);
    bool promotion = promoted(move);

    sideToMove = ~sideToMove;
    PieceType pt = piece(move);
    Piece p = makePiece(pt, sideToMove);

    const bool isCastling = (p == WhiteKing && capture == WhiteRook) ||
                            (p == BlackKing && capture == BlackRook);

    if (isCastling) {
        Square rookToSq = to_sq;
        Piece rook = sideToMove == White ? WhiteRook : BlackRook;
        Square rookFromSq = file_rank_square(to_sq > from_sq ? FILE_F : FILE_D,
                                             square_rank(from_sq));
        to_sq = file_rank_square(to_sq > from_sq ? FILE_G : FILE_C,
                                 square_rank(from_sq));

        // We need to remove both pieces first and then place them back.
        removePiece(rook, rookFromSq);
        removePiece(p, to_sq);

        placePiece(p, from_sq);
        placePiece(rook, rookToSq);
    } else if (promotion) {
        removePiece(p, to_sq);
        placePiece(makePiece(PAWN, sideToMove), from_sq);
        if (capture != None)
            placePiece(capture, to_sq);
        return;
    } else {
        movePiece(p, to_sq, from_sq);
    }

    if (to_sq == enPassantSquare && pt == PAWN) {
        int8_t offset = sideToMove == White ? -8 : 8;
        placePiece(makePiece(PAWN, ~sideToMove),
                   Square(enPassantSquare + offset));
    } else if (capture != None && !isCastling) {

        placePiece(capture, to_sq);
    }
}

void Board::applyFen(const std::string &fen) {
    for (Piece p = WhitePawn; p < None; p++) {
        piecesBB[p] = 0ULL;
    }

    pawnKey = 0ULL;

    const std::vector<std::string> params = splitInput(fen);

    const std::string position = params[0];
    const std::string move_right = params[1];
    const std::string castling = params[2];
    const std::string en_passant = params[3];

    const std::string half_move_clock = params.size() > 4 ? params[4] : "0";
    const std::string full_move_counter = params.size() > 4 ? params[5] : "1";

    sideToMove = (move_right == "w") ? White : Black;

    std::fill(std::begin(board), std::end(board), None);

    Square square = Square(56);
    for (int index = 0; index < static_cast<int>(position.size()); index++) {
        char curr = position[index];
        if (charToPiece.find(curr) != charToPiece.end()) {
            const Piece piece = charToPiece[curr];
            placePiece(piece, square);

            if (type_of_piece(piece) == PAWN) {
                pawnKey ^= updateKeyPiece(piece, square);
            }

            square = Square(square + 1);
        } else if (curr == '/')
            square = Square(square - 16);
        else if (isdigit(curr))
            square = Square(square + (curr - '0'));
    }

    removeCastlingRightsAll(White);
    removeCastlingRightsAll(Black);

    for (size_t i = 0; i < castling.size(); i++) {
        if (readCastleString.find(castling[i]) != readCastleString.end())
            castlingRights |= readCastleString[castling[i]];
    }

    if (en_passant == "-") {
        enPassantSquare = NO_SQ;
    } else {
        char letter = en_passant[0];
        int file = letter - 96;
        int rank = en_passant[1] - 48;
        enPassantSquare = Square((rank - 1) * 8 + file - 1);
    }

    halfMoveClock = std::stoi(half_move_clock);

    // full_move_counter actually half moves
    fullMoveNumber = std::stoi(full_move_counter) * 2;

    hashKey = zobristHash();

    hashHistory.clear();
    pawnKeyHistory.clear();
    stateHistory.clear();

    hashHistory.push_back(hashKey);
    pawnKeyHistory.push_back(pawnKey);

    Refresh();
}

void Board::makeNullMove() {
    stateHistory.emplace_back(
        State(enPassantSquare, castlingRights, halfMoveClock, None));
    sideToMove = ~sideToMove;

    hashKey ^= updateKeySideToMove();
    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);

    enPassantSquare = NO_SQ;
    fullMoveNumber++;
}

void Board::unmakeNullMove() {
    const State restore = stateHistory.back();
    stateHistory.pop_back();

    enPassantSquare = restore.enPassant;
    castlingRights = restore.castling;
    halfMoveClock = restore.halfMove;

    hashKey ^= updateKeySideToMove();
    if (enPassantSquare != NO_SQ)
        hashKey ^= updateKeyEnPassant(enPassantSquare);

    fullMoveNumber--;
    sideToMove = ~sideToMove;
}

void Board::removePiece(Piece piece, Square sq) {
    piecesBB[piece] &= ~(1ULL << sq);
    board[sq] = None;

    // nnue->EfficientlyUpdateAccumulator<NNUE::AccumulatorOperation::Deactivate>(type_of_piece(piece),
    // colorOf(sq), sq);
}

void Board::placePiece(Piece piece, Square sq) {
    piecesBB[piece] |= (1ULL << sq);
    board[sq] = piece;

    // nnue->EfficientlyUpdateAccumulator<NNUE::AccumulatorOperation::Activate>(type_of_piece(piece),
    // colorOf(sq), sq);
}

void Board::movePiece(Piece piece, Square fromSq, Square toSq) {
    piecesBB[piece] &= ~(1ULL << fromSq);
    piecesBB[piece] |= (1ULL << toSq);
    board[fromSq] = None;
    board[toSq] = piece;

    // nnue->EfficientlyUpdateAccumulator(type_of_piece(piece), colorOf(toSq),
    // fromSq, toSq);
}