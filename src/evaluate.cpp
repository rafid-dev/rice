#include "eval.h"
#include "psq.h"

#define SETBIT(bitboard, square) ((bitboard) |= (1ULL << (square)))

U64 FileMasks[64];
U64 RankMasks[64];
U64 IsolatedMasks[64];
U64 WhitePassedMasks[64];
U64 BlackPassedMasks[64];

Phase gamephaseInc[6] = {0, 1, 1, 2, 4, 0};

const int piece_values[12] = {100, 300, 300, 500, 900, 0};

#define S Score

Score PIECE_VALUES[] = {S(111, 80),  S(416, 307),  S(402, 306),
                        S(533, 565), S(1293, 944), S(0, 0)};
Score PAWN_TABLE[] = {
    S(0, 0),     S(0, 0),    S(0, 0),     S(0, 0),     S(0, 0),    S(0, 0),
    S(0, 0),     S(0, 0),    S(119, 173), S(146, 149), S(89, 140), S(145, 103),
    S(104, 120), S(186, 87), S(12, 155),  S(-24, 191), S(3, 94),   S(-6, 91),
    S(26, 70),   S(35, 46),  S(70, 23),   S(85, 29),   S(18, 64),  S(-4, 76),
    S(-10, 34),  S(1, 19),   S(3, 11),    S(28, -12),  S(29, -7),  S(21, 0),
    S(11, 8),    S(-14, 18), S(-27, 18),  S(-25, 11),  S(-2, -1),  S(17, -13),
    S(23, -9),   S(19, -8),  S(-2, -2),   S(-20, 3),   S(-18, 6),  S(-27, 6),
    S(-5, -4),   S(-5, 3),   S(10, 3),    S(18, -4),   S(23, -12), S(0, -8),
    S(-29, 22),  S(-20, 8),  S(-27, 19),  S(-16, 19),  S(-9, 19),  S(36, 1),
    S(32, -9),   S(-11, -3), S(0, 0),     S(0, 0),     S(0, 0),    S(0, 0),
    S(0, 0),     S(0, 0),    S(0, 0),     S(0, 0)};
Score KNIGHT_TABLE[] = {
    S(-204, -34), S(-115, -22), S(-55, 9),    S(-50, -11), S(86, -27),
    S(-109, -10), S(-23, -58),  S(-117, -89), S(-94, -7),  S(-55, 12),
    S(89, -19),   S(31, 16),    S(25, 5),     S(84, -20),  S(-3, -10),
    S(-19, -40),  S(-48, -15),  S(74, -11),   S(52, 25),   S(83, 23),
    S(114, 2),    S(162, -5),   S(101, -14),  S(64, -41),  S(3, -7),
    S(39, 14),    S(31, 42),    S(77, 35),    S(59, 36),   S(98, 22),
    S(42, 17),    S(42, -11),   S(0, -8),     S(26, 4),    S(41, 28),
    S(33, 41),    S(52, 30),    S(44, 30),    S(43, 16),   S(8, -8),
    S(-10, -18),  S(8, 10),     S(34, 5),     S(41, 22),   S(54, 19),
    S(47, -0),    S(47, -11),   S(1, -21),    S(-11, -41), S(-36, -12),
    S(7, 3),      S(28, 4),     S(31, 8),     S(42, -12),  S(2, -17),
    S(4, -41),    S(-121, -8),  S(-2, -38),   S(-37, -8),  S(-17, -1),
    S(13, -13),   S(-3, -11),   S(3, -38),    S(-12, -71)};
Score BISHOP_TABLE[] = {
    S(-28, -2), S(-6, -9),  S(-145, 19), S(-89, 13), S(-66, 20), S(-57, 9),
    S(-31, 5),  S(2, -14),  S(-34, 10),  S(-1, 4),   S(-35, 17), S(-52, 5),
    S(18, 4),   S(48, -2),  S(2, 6),     S(-51, 0),  S(-18, 14), S(31, -0),
    S(37, 2),   S(27, -0),  S(31, 0),    S(57, 3),   S(33, 7),   S(4, 14),
    S(-0, 5),   S(12, 12),  S(12, 15),   S(51, 7),   S(35, 10),  S(30, 8),
    S(13, 3),   S(8, 10),   S(5, -2),    S(20, 4),   S(12, 13),  S(34, 13),
    S(39, -1),  S(9, 6),    S(14, -1),   S(22, -6),  S(13, -8),  S(31, -1),
    S(29, 7),   S(16, 7),   S(24, 12),   S(46, -5),  S(31, -1),  S(24, -13),
    S(20, -10), S(40, -21), S(30, -7),   S(16, 3),   S(27, 1),   S(37, -3),
    S(61, -26), S(19, -27), S(-20, -10), S(20, -2),  S(13, 2),   S(5, 4),
    S(9, 3),    S(10, 6),   S(-29, 8),   S(-5, -11)};
Score ROOK_TABLE[] = {
    S(10, 28),  S(24, 21),  S(-14, 37), S(41, 19),  S(44, 21),  S(-14, 31),
    S(2, 28),   S(17, 22),  S(12, 27),  S(6, 31),   S(50, 19),  S(55, 17),
    S(80, -2),  S(87, 6),   S(-1, 31),  S(36, 18),  S(-26, 27), S(-7, 26),
    S(-7, 23),  S(-4, 24),  S(-27, 25), S(48, 4),   S(77, -0),  S(8, 10),
    S(-34, 24), S(-29, 22), S(-17, 32), S(-4, 17),  S(-7, 19),  S(32, 12),
    S(-4, 11),  S(-20, 20), S(-40, 22), S(-31, 24), S(-23, 25), S(-19, 20),
    S(-0, 10),  S(-3, 9),   S(21, 2),   S(-22, 8),  S(-32, 12), S(-18, 16),
    S(-13, 7),  S(-15, 12), S(5, 3),    S(17, -1),  S(12, 3),   S(-11, -4),
    S(-22, 6),  S(-7, 8),   S(-14, 14), S(1, 13),   S(14, 1),   S(29, -2),
    S(15, -4),  S(-43, 11), S(8, 9),    S(7, 12),   S(14, 11),  S(26, 4),
    S(31, -2),  S(36, 0),   S(-7, 10),  S(12, -9)};
Score QUEEN_TABLE[] = {
    S(-30, 3),   S(-27, 49), S(-8, 42),  S(2, 37),   S(113, -8), S(110, -14),
    S(78, -8),   S(57, 38),  S(-30, 1),  S(-58, 30), S(-21, 40), S(-20, 57),
    S(-54, 86),  S(59, 18),  S(20, 39),  S(64, 13),  S(-3, -22), S(-22, 2),
    S(6, -17),   S(-19, 56), S(25, 36),  S(77, 15),  S(62, 17),  S(67, 22),
    S(-37, 25),  S(-28, 17), S(-25, 14), S(-22, 23), S(-6, 46),  S(3, 44),
    S(-4, 78),   S(0, 71),   S(2, -28),  S(-31, 29), S(-0, -7),  S(-11, 33),
    S(2, 7),     S(4, 22),   S(2, 54),   S(5, 42),   S(-11, 11), S(16, -40),
    S(4, -3),    S(18, -34), S(13, -8),  S(16, 9),   S(22, 24),  S(12, 43),
    S(-17, -12), S(13, -36), S(30, -39), S(30, -40), S(41, -47), S(44, -38),
    S(17, -37),  S(32, -30), S(18, -32), S(17, -47), S(27, -42), S(35, -7),
    S(16, -8),   S(-1, -17), S(-5, -6),  S(-37, -29)};
Score KING_TABLE[] = {
    S(-109, -78), S(188, -74), S(180, -57), S(102, -46), S(-151, 13),
    S(-84, 31),   S(50, -6),   S(64, -29),  S(228, -60), S(67, 4),
    S(37, 8),     S(113, -2),  S(30, 14),   S(43, 34),   S(-34, 30),
    S(-150, 38),  S(75, -6),   S(73, 11),   S(97, 10),   S(15, 16),
    S(37, 14),    S(110, 36),  S(134, 28),  S(-10, 11),  S(11, -19),
    S(-14, 24),   S(9, 28),    S(-54, 40),  S(-58, 39),  S(-60, 47),
    S(-23, 35),   S(-97, 16),  S(-90, -9),  S(14, -5),   S(-74, 37),
    S(-136, 50),  S(-141, 55), S(-95, 42),  S(-89, 25),  S(-105, 1),
    S(-4, -26),   S(-17, 0),   S(-57, 23),  S(-98, 38),  S(-96, 42),
    S(-83, 34),   S(-34, 14),  S(-50, -4),  S(5, -38),   S(9, -13),
    S(-41, 16),   S(-101, 28), S(-77, 28),  S(-54, 19),  S(2, -4),
    S(11, -27),   S(-21, -67), S(43, -50),  S(12, -27),  S(-95, 2),
    S(-14, -21),  S(-50, -5),  S(30, -38),  S(19, -64)};

Score OPEN_FILE_BONUS[] = {S(0, 0),   S(0, 0), S(0, 0),
                           S(59, -8), S(0, 0), S(0, 0)};
Score SEMI_OPEN_FILE_BONUS[] = {S(0, 0),  S(0, 0), S(0, 0),
                                S(19, 4), S(0, 0), S(0, 0)};

Score BISHOP_PAIR_BONUS = S(32, 54);
Score KNIGHT_MOBILITY = S(1, 0);
Score BISHOP_MOBILITY = S(6, 3);
Score ROOK_MOBILITY = S(5, 3);
Score QUEEN_MOBILITY = S(2, 8);

Score KING_PAWN_SHIELD[] = {S(40, -13), S(34, -10)};

#undef S

Score *psqt[6] = {PAWN_TABLE, KNIGHT_TABLE, BISHOP_TABLE,
                  ROOK_TABLE, QUEEN_TABLE,  KING_TABLE};

Score PsqTable[12][64];

static U64 SetFileRankMask(int file_number, int rank_number) {
  U64 mask = 0ULL;

  for (int rank = 0; rank < 8; rank++) {
    for (int file = 0; file < 8; file++) {
      int square = rank * 8 + file;

      if (file_number != -1) {
        if (file == file_number)
          mask |= SETBIT(mask, square);
      }

      else if (rank_number != -1) {
        if (rank == rank_number)
          mask |= SETBIT(mask, square);
      }
    }
  }

  return mask;
}

void InitEvaluationMasks() {

  // Init files and ranks masks
  for (int rank = 0; rank < 8; rank++) {
    for (int file = 0; file < 8; file++) {
      int square = rank * 8 + file;

      FileMasks[square] |= SetFileRankMask(file, -1);
    }
  }
  for (int rank = 0; rank < 8; rank++) {
    for (int file = 0; file < 8; file++) {
      int square = rank * 8 + file;

      RankMasks[square] |= SetFileRankMask(-1, rank);
    }
  }

  // Init isolated pawn masks
  for (int rank = 0; rank < 8; rank++) {
    for (int file = 0; file < 8; file++) {
      int square = rank * 8 + file;
      IsolatedMasks[square] |= SetFileRankMask(file - 1, -1);
      IsolatedMasks[square] |= SetFileRankMask(file + 1, -1);
    }
  }

  // Init passed pawn masks for White
  for (int rank = 0; rank < 8; rank++) {
    for (int file = 0; file < 8; file++) {
      int square = rank * 8 + file;
      WhitePassedMasks[square] |= SetFileRankMask(file - 1, -1);
      WhitePassedMasks[square] |= SetFileRankMask(file, -1);
      WhitePassedMasks[square] |= SetFileRankMask(file + 1, -1);

      for (int i = 0; i < (8 - rank); i++) {
        WhitePassedMasks[square] &= ~RankMasks[(7 - i) * 8 + file];
      }
    }
  }

  // Init passed pawn masks for Black
  for (int rank = 0; rank < 8; rank++) {
    for (int file = 0; file < 8; file++) {
      int square = rank * 8 + file;
      BlackPassedMasks[square] |= SetFileRankMask(file - 1, -1);
      BlackPassedMasks[square] |= SetFileRankMask(file, -1);
      BlackPassedMasks[square] |= SetFileRankMask(file + 1, -1);

      for (int i = 0; i < rank + 1; i++) {
        BlackPassedMasks[square] &= ~RankMasks[i * 8 + file];
      }
    }
  }

  // std::cout << "U64 FileMasks[64] = {\n";
  // for (int rank = 0; rank < 8; rank++){
  //     for (int file = 0; file < 8; file++){
  //         int square = rank * 8 + file;
  //         std::cout << FileMasks[square] << "ULL,";
  //     }
  //     std::cout << "\n";
  // }
  // std::cout << "}\n";
}

void InitPsqTables() {

  for (Piece pc = WhitePawn; pc < BlackPawn; pc++) {
    PieceType p = PieceToPieceType[pc];
    for (Square sq = SQ_A1; sq < NO_SQ; sq++) {
      PsqTable[pc][sq] = PIECE_VALUES[p] + psqt[p][sq ^ 56];
      PsqTable[pc + 6][sq] = PIECE_VALUES[p] + psqt[p][sq];
    }
  }
}

// Check if a file is semi open for a given side
static inline bool IsSemiOpenFile(Board &board, Square sq, Color us) {
  return (((us == White) ? board.piecesBB[WhitePawn]
                         : board.piecesBB[BlackPawn]) &
          FileMasks[sq]) == 0;
}

// Check if a file is open
static inline bool IsOpenFile(Board &board, Square sq) {
  return ((board.piecesBB[WhitePawn] | board.piecesBB[BlackPawn]) &
          FileMasks[sq]) == 0;
}

template <Color c> Score EvaluatePawnPsqt(Board &board) {
  Score score;
  Square sq;
  Piece p = (c == White) ? WhitePawn : BlackPawn;
  U64 piece_bb = board.piecesBB[p];

  while (piece_bb) {
    sq = poplsb(piece_bb);

    // Evaluate piece square tables
    score += PsqTable[p][sq];
  }

  return score;
}

template <Color c>
Score EvaluateKnights(Board &board, U64 all, int8_t &gamePhase) {
  Score score;
  Square sq;
  Piece p = (c == White) ? WhiteKnight : BlackKnight;
  U64 piece_bb = board.piecesBB[p];

  while (piece_bb) {
    sq = poplsb(piece_bb);

    // Evaluate piece square tables
    score += PsqTable[p][sq];

    // Add gamephase
    gamePhase += gamephaseInc[type_of_piece(p)];

    // Evaluate mobility
    score += KNIGHT_MOBILITY * popcount(KnightAttacks(sq) & all);
  }

  return score;
}

template <Color c>
Score EvaluateBishops(Board &board, U64 all, int8_t &gamePhase) {
  Score score;
  Square sq;
  Piece p = (c == White) ? WhiteBishop : BlackBishop;
  U64 piece_bb = board.piecesBB[p];

  while (piece_bb) {
    sq = poplsb(piece_bb);

    // Evaluate piece square tables
    score += PsqTable[p][sq];

    // Add gamephase
    gamePhase += gamephaseInc[type_of_piece(p)];

    // Evaluate mobility
    score += BISHOP_MOBILITY * popcount(BishopAttacks(sq, all));
  }

  if (popcount(piece_bb) > 1) {
    score += BISHOP_PAIR_BONUS;
  }

  return score;
}

template <Color c> Score EvaluateRooks(Board &board, U64 all, int8_t &gamePhase) {
  Score score;
  Square sq;
  Piece p = (c == White) ? WhiteRook : BlackRook;
  U64 piece_bb = board.piecesBB[p];

  while (piece_bb) {
    sq = poplsb(piece_bb);

    // Evaluate piece square tables
    score += PsqTable[p][sq];

    // Add gamephase
    gamePhase += gamephaseInc[type_of_piece(p)];

    // Evaluate Mobility
    score += ROOK_MOBILITY * popcount(RookAttacks(sq, all));

    // Evaluate oppen file and semi open file
    if (IsOpenFile(board, sq)) {
      score += OPEN_FILE_BONUS[ROOK];
    } else if (IsSemiOpenFile(board, sq, c)) {
      score += SEMI_OPEN_FILE_BONUS[ROOK];
    }
  }

  return score;
}

template <Color c> Score EvaluateQueens(Board &board, U64 all, int8_t &gamePhase) {
  Score score;
  Square sq;
  Piece p = (c == White) ? WhiteQueen : BlackQueen;
  U64 piece_bb = board.piecesBB[p];

  while (piece_bb) {
    sq = poplsb(piece_bb);

    // Evaluate piece square tables
    score += PsqTable[p][sq];

    // Add gamephase
    gamePhase += gamephaseInc[type_of_piece(p)];

    // Evaluate Mobility
    score += QUEEN_MOBILITY * popcount(QueenAttacks(sq, all));

  //   if (IsOpenFile(board, sq)){
  //   score += OPEN_FILE_BONUS[QUEEN];
  // }else if(IsSemiOpenFile(board, sq, c)){
  //   score += SEMI_OPEN_FILE_BONUS[QUEEN];
  // }
  }

  return score;
}

template <Color c> Score EvaluateKings(Board &board) {
  Score score;
  Piece p = (c == White) ? WhiteKing : BlackKing;
  U64 piece_bb = board.piecesBB[p];
  // U64 pawns = board.piecesBB[c == White ? WhitePawn : BlackPawn];

  Square sq = poplsb(piece_bb);
  // File file = square_file(sq);
  // const int kingside = file >> 2;

  // Evaluate piece square tables
  score += PsqTable[p][sq];

  // if (IsOpenFile(board, sq)){
  //   score += OPEN_FILE_BONUS[KING];
  // }else if(IsSemiOpenFile(board, sq, c)){
  //   score += SEMI_OPEN_FILE_BONUS[KING];
  // }

  return score;
}

template <Color c> Score EvaluateSide(Board& board, int8_t& gamePhase){
  
}

int Evaluate(Board &board, PawnTable &pawnTable) {
  // Score //
  Score score[2];

  // Game phase //
  int8_t gamePhase = 0;

  Color side2move = board.sideToMove;
  Color otherSide = ~board.sideToMove;

  // Probe pawn evaluation entry ///
  PawnEntry *pe = pawnTable.probeEntry(board);

  // All occupancies on the board //
  U64 all = board.All();

  // Evaluate white pieces //
  score[White] += EvaluatePawnPsqt<White>(board);
  score[White] += EvaluateKnights<White>(board, all, gamePhase);
  score[White] += EvaluateBishops<White>(board, all, gamePhase);
  score[White] += EvaluateRooks<White>(board, all, gamePhase);
  score[White] += EvaluateQueens<White>(board, all, gamePhase);
  score[White] += EvaluateKings<White>(board);

  // Evaluate black pieces //
  score[Black] += EvaluatePawnPsqt<Black>(board);
  score[Black] += EvaluateKnights<Black>(board, all, gamePhase);
  score[Black] += EvaluateBishops<Black>(board, all, gamePhase);
  score[Black] += EvaluateRooks<Black>(board, all, gamePhase);
  score[Black] += EvaluateQueens<Black>(board, all, gamePhase);
  score[Black] += EvaluateKings<Black>(board);

  // tapered eval //
  int16_t mgScore = (score[side2move].mg - score[otherSide].mg);
  int16_t egScore = (score[side2move].eg - score[otherSide].eg);

  // add pawn evaluation scores //
  mgScore += pe->value.mg;
  egScore += pe->value.eg;

  int16_t mgPhase = gamePhase;
  if (mgPhase > 24)
    mgPhase = 24;
  int16_t egPhase = 24 - mgPhase;

  return (mgScore * mgPhase + egScore * egPhase) / 24;
}