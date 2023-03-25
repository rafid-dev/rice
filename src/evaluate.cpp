#include "eval.h"
#include "psq.h"

#define SETBIT(bitboard, square) ((bitboard) |= (1ULL << (square)))

// rook open file bonus
Score rook_semi_open_file(0, 0);
Score rook_open_file(33, 2);

// king safety
Score king_shelter(30, -12);

// bishop pair
Score bishop_pair_bonus(18, 42);

U64 FileMasks[64];
U64 RankMasks[64];
U64 IsolatedMasks[64];
U64 WhitePassedMasks[64];
U64 BlackPassedMasks[64];

int gamephaseInc[6] = {0, 1, 1, 2, 4, 0};

int mg_value[6] = {80, 335, 364, 475, 1025, 0};
int eg_value[6] = {91, 280, 296, 509, 940, 0};

const int piece_values[12] = {100, 300, 300, 500, 900, 0};

#define S Score

// Bishop Bonuses
Score BISHOP_PAIR_BONUS = S(32, 52);

Score OPEN_FILE_BONUS[] = {S(0, 0),   S(0, 0), S(0, 0),
                           S(59, -8), S(0, 0), S(0, 0)};
Score SEMI_OPEN_FILE_BONUS[] = {S(0, 0),  S(0, 0), S(0, 0),
                                S(19, 4), S(0, 0), S(0, 0)};

Score PIECE_VALUES[6] = {
    S(89, 77), S(379, 290), S(388, 307), S(533, 549), S(1163, 998), S(0, 0),
};

Score PAWN_TABLE[NSQUARES] = {
    S(0, 0),    S(0, 0),    S(0, 0),     S(0, 0),     S(0, 0),    S(0, 0),
    S(0, 0),    S(0, 0),    S(104, 149), S(145, 126), S(80, 115), S(129, 81),
    S(93, 96),  S(159, 73), S(25, 128),  S(-25, 166), S(-2, 86),  S(3, 80),
    S(32, 58),  S(39, 30),  S(77, 9),    S(89, 19),   S(27, 53),  S(-5, 68),
    S(-14, 31), S(6, 15),   S(13, 5),    S(32, -18),  S(34, -11), S(24, -3),
    S(17, 5),   S(-12, 15), S(-30, 17),  S(-15, 8),   S(-2, -2),  S(20, -14),
    S(25, -10), S(15, -8),  S(4, -4),    S(-18, 2),   S(-27, 7),  S(-18, 4),
    S(-1, -6),  S(-8, 3),   S(7, 3),     S(12, -4),   S(29, -14), S(-3, -7),
    S(-39, 23), S(-17, 6),  S(-21, 15),  S(-24, 20),  S(-16, 19), S(32, 0),
    S(32, -9),  S(-16, -1), S(0, 0),     S(0, 0),     S(0, 0),    S(0, 0),
    S(0, 0),    S(0, 0),    S(0, 0),     S(0, 0),
};
Score KNIGHT_TABLE[NSQUARES] = {
    S(-205, -40), S(-106, -30), S(-37, -4),   S(-38, -23), S(95, -37),
    S(-110, -15), S(-15, -61),  S(-118, -91), S(-77, -17), S(-44, 1),
    S(85, -26),   S(38, 4),     S(32, -5),    S(82, -28),  S(5, -19),
    S(-6, -49),   S(-40, -21),  S(77, -20),   S(52, 14),   S(82, 11),
    S(112, -9),   S(154, -14),  S(97, -23),   S(65, -47),  S(-5, -13),
    S(30, 8),     S(31, 30),    S(71, 26),    S(55, 26),   S(90, 12),
    S(33, 9),     S(33, -15),   S(-4, -11),   S(17, -1),   S(28, 22),
    S(24, 33),    S(41, 22),    S(34, 22),    S(33, 10),   S(3, -13),
    S(-14, -19),  S(1, 6),      S(25, 0),     S(24, 19),   S(33, 15),
    S(33, -4),    S(39, -16),   S(-5, -25),   S(-23, -35), S(-54, -10),
    S(-2, -3),    S(7, 2),      S(9, 6),      S(32, -18),  S(-8, -20),
    S(-8, -43),   S(-128, -8),  S(-13, -46),  S(-57, -13), S(-31, -9),
    S(-5, -19),   S(-19, -16),  S(-9, -44),   S(-13, -71)};
Score BISHOP_TABLE[NSQUARES] = {
    S(-33, -9), S(5, -17),  S(-121, 6),  S(-62, 1),  S(-47, 8),  S(-44, -1),
    S(-2, -6),  S(-3, -23), S(-24, -2),  S(14, 0),   S(-15, 12), S(-25, -1),
    S(41, -1),  S(65, -7),  S(20, 1),    S(-45, -7), S(-11, 6),  S(45, -6),
    S(55, 2),   S(49, 1),   S(45, 1),    S(75, 4),   S(45, 3),   S(6, 7),
    S(3, 1),    S(15, 13),  S(29, 16),   S(61, 14),  S(49, 17),  S(47, 11),
    S(16, 5),   S(3, 7),    S(-2, -1),   S(22, 7),   S(22, 18),  S(35, 25),
    S(46, 11),  S(22, 10),  S(18, 0),    S(10, -5),  S(7, -7),   S(25, 2),
    S(22, 14),  S(25, 13),  S(22, 17),   S(39, 4),   S(27, -3),  S(16, -13),
    S(13, -10), S(26, -20), S(26, -5),   S(7, 6),    S(18, 5),   S(27, -4),
    S(46, -24), S(13, -30), S(-37, -17), S(2, -3),   S(-9, -12), S(-17, -1),
    S(-11, -4), S(-7, -5),  S(-35, 2),   S(-23, -13)};
Score ROOK_TABLE[NSQUARES] = {
    S(49, 18),   S(70, 11),  S(40, 25),  S(96, 9),   S(92, 11),  S(11, 22),
    S(28, 18),   S(51, 11),  S(48, 14),  S(44, 18),  S(88, 9),   S(92, 9),
    S(115, -11), S(104, -1), S(36, 15),  S(62, 6),   S(-1, 17),  S(27, 15),
    S(35, 12),   S(43, 13),  S(21, 12),  S(68, -5),  S(90, -9),  S(26, 1),
    S(-23, 17),  S(-9, 14),  S(10, 23),  S(33, 8),   S(28, 10),  S(41, 5),
    S(4, 4),     S(-15, 13), S(-43, 20), S(-27, 19), S(-11, 20), S(-0, 16),
    S(8, 7),     S(-7, 5),   S(17, -2),  S(-25, 3),  S(-51, 14), S(-21, 13),
    S(-14, 6),   S(-20, 13), S(3, 2),    S(-1, -1),  S(1, -0),   S(-33, -3),
    S(-50, 9),   S(-13, 5),  S(-20, 12), S(-11, 15), S(0, 1),    S(8, 1),
    S(-3, -3),   S(-74, 12), S(-23, 9),  S(-14, 16), S(1, 16),   S(12, 12),
    S(15, 5),    S(-3, 6),   S(-31, 15), S(-24, -7)};
Score QUEEN_TABLE[NSQUARES] = {
    S(-38, 2),   S(-5, 34),   S(14, 34),   S(15, 35),   S(123, -14),
    S(127, -30), S(67, -9),   S(49, 29),   S(-19, -18), S(-45, 31),
    S(-8, 43),   S(3, 52),    S(-32, 86),  S(75, 15),   S(27, 37),
    S(67, -7),   S(-3, -27),  S(-11, 5),   S(12, 7),    S(-1, 67),
    S(31, 56),   S(76, 28),   S(62, 14),   S(64, 5),    S(-31, 14),
    S(-27, 36),  S(-15, 31),  S(-15, 54),  S(-3, 73),   S(9, 62),
    S(-2, 77),   S(-1, 55),   S(-5, -26),  S(-28, 39),  S(-7, 23),
    S(-11, 62),  S(-2, 39),   S(0, 39),    S(2, 51),    S(2, 21),
    S(-18, 3),   S(8, -32),   S(-8, 22),   S(3, -3),    S(-2, 16),
    S(5, 19),    S(17, 12),   S(3, 24),    S(-35, -18), S(-4, -27),
    S(17, -33),  S(7, -23),   S(15, -27),  S(24, -36),  S(1, -39),
    S(8, -39),   S(-1, -34),  S(-15, -31), S(-5, -27),  S(13, -24),
    S(-15, -3),  S(-28, -30), S(-42, -6),  S(-50, -46)};
Score KING_TABLE[NSQUARES] = {
    S(-76, -73), S(169, -68), S(150, -50), S(77, -38),  S(-150, 13),
    S(-88, 28),  S(32, -2),   S(47, -24),  S(201, -54), S(53, 6),
    S(19, 8),    S(102, -4),  S(31, 11),   S(29, 31),   S(-42, 28),
    S(-138, 35), S(68, -7),   S(62, 9),    S(78, 9),    S(19, 11),
    S(25, 10),   S(101, 29),  S(114, 24),  S(-19, 11),  S(12, -19),
    S(-21, 22),  S(9, 22),    S(-58, 36),  S(-54, 33),  S(-53, 41),
    S(-13, 28),  S(-87, 14),  S(-86, -10), S(9, -7),    S(-72, 33),
    S(-125, 45), S(-131, 48), S(-87, 38),  S(-79, 21),  S(-100, 2),
    S(-3, -23),  S(-19, -0),  S(-50, 19),  S(-89, 34),  S(-84, 37),
    S(-74, 30),  S(-26, 10),  S(-48, -4),  S(4, -34),   S(7, -14),
    S(-31, 11),  S(-96, 25),  S(-75, 25),  S(-45, 14),  S(6, -7),
    S(15, -27),  S(-17, -63), S(41, -50),  S(5, -27),   S(-92, -2),
    S(-9, -28),  S(-53, -8),  S(26, -37),  S(23, -62)};

Score KNIGHT_MOBILITY = S(1, 0);
Score BISHOP_MOBILITY = S(6, 3);
Score ROOK_MOBILITY = S(5, 3);
Score QUEEN_MOBILITY = S(2, 8);

#undef S

Score *psqt[6] = {PAWN_TABLE, KNIGHT_TABLE, BISHOP_TABLE,
                  ROOK_TABLE, QUEEN_TABLE,  KING_TABLE};

Score PestoTable[12][64];

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

void InitPestoTables() {

  for (Piece pc = WhitePawn; pc < BlackPawn; pc++) {
    PieceType p = PieceToPieceType[pc];
    for (Square sq = SQ_A1; sq < NO_SQ; sq++) {
      PestoTable[pc][sq] = PIECE_VALUES[p] + psqt[p][sq ^ 56];
      PestoTable[pc + 6][sq] = PIECE_VALUES[p] + psqt[p][sq];
    }
  }

  /*for (Piece i = WhitePawn; i < BlackPawn; i++){

      switch(i){
          case WhitePawn:
          std::cout << "const Score PawnTable[64]";
          break;
          case WhiteKnight:
          std::cout << "const Score KnightTable[64]";
          break;
          case WhiteBishop:
          std::cout << "const Score BishopTable[64]";
          break;
          case WhiteRook:
          std::cout << "const Score RookTable[64]";
          break;
          case WhiteQueen:
          std::cout << "const Score QueenTable[64]";
          break;
          case WhiteKing:
          std::cout << "const Score KingTable[64]";
          break;
      }
      std::cout << " = {\n";
      for (int rank = 0; rank < 8; rank++){
          for (int file = 0; file < 8; file++){
              int sq = rank * 8 + file;
              std::cout << "S(" << PestoTable[i][sq].mg << "," <<
  PestoTable[i][sq].eg << "),";
          }
          std::cout << "\n";
      }
      std::cout << "};\n";
  }*/
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

int Evaluate(Board &board, PawnTable &pawnTable) {
  Score score[2];

  int gamePhase = 0;

  Color side2move = board.sideToMove;
  Color otherSide = ~board.sideToMove;

  U64 white_pieces = board.Us(White);
  U64 black_pieces = board.Us(Black);

  PawnEntry *pe = pawnTable.probeEntry(board);

  U64 occ_white = board.Us(White);
  U64 occ_black = board.Us(Black);
  U64 all = board.All();

  while (white_pieces) {
    Square sq = poplsb(white_pieces);
    Piece p = board.pieceAtB(sq);

    score[White] += PestoTable[p][sq];

    gamePhase += gamephaseInc[type_of_piece(p)];

    /*if (p == WhiteKnight) {
      score[White] += KNIGHT_MOBILITY * popcount(KnightAttacks(sq) & all);
    } else if (p == WhiteRook) {
      score[White] += ROOK_MOBILITY * popcount(RookAttacks(sq, all));

      if (IsOpenFile(board, sq)) {
        score[White] += OPEN_FILE_BONUS[ROOK];
      } else if (IsSemiOpenFile(board, sq, White)) {
        score[White] += SEMI_OPEN_FILE_BONUS[ROOK];
      }

    } else if (p == WhiteBishop) {
      score[White] += BISHOP_MOBILITY * popcount(BishopAttacks(sq, all));
    } else if (p == WhiteQueen) {
      score[White] += QUEEN_MOBILITY * popcount(QueenAttacks(sq, all));
    }*/
  }

  while (black_pieces) {
    Square sq = poplsb(black_pieces);
    Piece p = board.pieceAtB(sq);

    score[Black] += PestoTable[p][sq];

    gamePhase += gamephaseInc[type_of_piece(p)];
    /*if (p == BlackKnight) {
      score[Black] += KNIGHT_MOBILITY * popcount((KnightAttacks(sq) & all));
    } else if (p == BlackRook) {
      score[Black] += ROOK_MOBILITY * popcount((RookAttacks(sq, all)));

      if (IsOpenFile(board, sq)) {
        score[Black] += OPEN_FILE_BONUS[ROOK];
      } else if (IsSemiOpenFile(board, sq, Black)) {
        score[Black] += SEMI_OPEN_FILE_BONUS[ROOK];
      }

    } else if (p == BlackBishop) {
      score[Black] += BISHOP_MOBILITY * popcount(BishopAttacks(sq, all));
    } else if (p == BlackQueen) {
      score[Black] += QUEEN_MOBILITY * popcount(QueenAttacks(sq, all));
    }*/
  }

  if (popcount(board.piecesBB[WhiteBishop]) >= 1) {
    score[White] += BISHOP_PAIR_BONUS;
  }
  if (popcount(board.piecesBB[BlackBishop]) >= 1) {
    score[Black] += BISHOP_PAIR_BONUS;
  }

  // tapered eval //
  int16_t mgScore = (score[side2move].mg - score[otherSide].mg);
  int16_t egScore = (score[side2move].eg - score[otherSide].eg);

  mgScore += pe->value.mg;
  egScore += pe->value.eg;

  int16_t mgPhase = gamePhase;
  if (mgPhase > 24)
    mgPhase = 24;
  int16_t egPhase = 24 - mgPhase;

  return (mgScore * mgPhase + egScore * egPhase) / 24;
}