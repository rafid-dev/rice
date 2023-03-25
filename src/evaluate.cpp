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

int gamephaseInc[12] = {0, 0, 1, 1, 1, 1, 2, 2, 4, 4, 0, 0};

int mg_value[6] = {80, 335, 364, 475, 1025, 0};
int eg_value[6] = {91, 280, 296, 509, 940, 0};

const int piece_values[12] = {100, 300, 300, 500, 900, 0};

#define S Score
Score material_values[6] = {S(64, 100),  S(245, 218), S(250, 236),
                            S(323, 427), S(716, 786), S(0, 0)};
Score psqt[6][64] = {
    {// pawns
     S(0, 0),     S(0, 0),     S(0, 0),     S(0, 0),     S(0, 0),
     S(0, 0),     S(0, 0),     S(0, 0),     S(67, 122),  S(92, 115),
     S(46, 107),  S(83, 80),   S(54, 93),   S(66, 77),   S(8, 116),
     S(-32, 138), S(-11, 59),  S(-3, 67),   S(19, 47),   S(28, 29),
     S(43, 19),   S(49, 15),   S(26, 45),   S(-6, 47),   S(-23, 9),
     S(1, -1),    S(-1, -11),  S(8, -24),   S(17, -29),  S(10, -22),
     S(13, -11),  S(-18, -8),  S(-32, -7),  S(-10, -12), S(-12, -24),
     S(3, -31),   S(4, -32),   S(-2, -29),  S(6, -21),   S(-22, -22),
     S(-32, -12), S(-12, -14), S(-12, -27), S(-17, -6),  S(-6, -17),
     S(-4, -25),  S(19, -24),  S(-14, -28), S(-37, -7),  S(-10, -12),
     S(-22, -17), S(-27, -5),  S(-19, -4),  S(9, -20),   S(23, -21),
     S(-21, -25), S(0, 0),     S(0, 0),     S(0, 0),     S(0, 0),
     S(0, 0),     S(0, 0),     S(0, 0),     S(0, 0)},
    {// knights
     S(-113, -38), S(-54, -22), S(-49, 6),   S(-12, -14), S(57, -19),
     S(-65, -7),   S(-25, -32), S(-70, -71), S(-40, -8),  S(-23, 7),
     S(50, -9),    S(29, 7),    S(25, 1),    S(56, -11),  S(9, -6),
     S(1, -27),    S(-32, -4),  S(39, -2),   S(37, 17),   S(56, 12),
     S(70, 3),     S(91, 2),    S(51, -5),   S(27, -21),  S(-3, 1),
     S(8, 17),     S(18, 29),   S(43, 28),   S(26, 28),   S(55, 21),
     S(14, 16),    S(24, -5),   S(-15, 6),   S(1, 9),     S(11, 29),
     S(9, 32),     S(18, 30),   S(11, 29),   S(22, 11),   S(-7, 3),
     S(-22, -3),   S(-10, 16),  S(6, 3),     S(5, 23),    S(16, 22),
     S(9, 7),      S(13, 0),    S(-18, -3),  S(-28, -21), S(-29, 0),
     S(-12, 5),    S(-7, 19),   S(-6, 12),   S(8, -3),    S(-6, -11),
     S(-10, -21),  S(-91, -9),  S(-22, -25), S(-36, -5),  S(-26, -2),
     S(-22, 1),    S(-18, -2),  S(-20, -10), S(-24, -37)},
    {// bishops
     S(-12, -6), S(-13, -9),  S(-67, 5),   S(-72, 8),  S(-46, 1),  S(-51, -1),
     S(-12, -8), S(-28, -10), S(-12, -5),  S(11, 0),   S(-14, 4),  S(-17, -2),
     S(28, -3),  S(25, -6),   S(18, -1),   S(-27, -9), S(-15, 3),  S(20, -1),
     S(26, 3),   S(28, 0),    S(38, -4),   S(51, 0),   S(30, 1),   S(13, -3),
     S(-8, 1),   S(-1, 11),   S(14, 14),   S(36, 13),  S(24, 15),  S(32, 6),
     S(3, 3),    S(-8, 8),    S(-10, -3),  S(2, 9),    S(4, 12),   S(18, 16),
     S(19, 11),  S(5, 6),     S(2, 5),     S(-10, -4), S(-2, -4),  S(9, -4),
     S(5, 12),   S(8, 5),     S(7, 9),     S(9, 9),    S(5, 0),    S(2, -8),
     S(5, -20),  S(4, -3),    S(11, -13),  S(-6, 9),   S(-1, 12),  S(12, -6),
     S(18, -5),  S(0, -19),   S(-23, -16), S(-1, -9),  S(-16, -4), S(-18, -2),
     S(-11, -4), S(-17, 2),   S(-8, -10),  S(-17, -13)},
    {// rooks
     S(11, 10),  S(28, 4),   S(-1, 17),  S(28, 6),   S(32, 6),   S(18, 8),
     S(31, 1),   S(44, 0),   S(-2, 11),  S(0, 13),   S(19, 9),   S(33, 4),
     S(46, -7),  S(52, -4),  S(5, 9),    S(17, 4),   S(-20, 9),  S(2, 8),
     S(-2, 9),   S(7, 5),    S(3, 1),    S(45, -10), S(50, -9),  S(19, -6),
     S(-27, 7),  S(0, -1),   S(-11, 10), S(-2, 3),   S(0, 0),    S(16, -1),
     S(9, -6),   S(2, -3),   S(-32, 4),  S(-25, 5),  S(-22, 8),  S(-15, 4),
     S(-2, -6),  S(-7, -4),  S(8, -8),   S(-9, -9),  S(-33, -2), S(-21, 1),
     S(-22, -3), S(-20, -4), S(-15, -3), S(-4, -10), S(12, -16), S(-15, -17),
     S(-35, -3), S(-19, -5), S(-21, -1), S(-19, 1),  S(-13, -6), S(0, -7),
     S(5, -13),  S(-45, -4), S(-15, 0),  S(-14, 3),  S(-13, 6),  S(-4, -3),
     S(-2, -5),  S(-5, 1),   S(-17, 0),  S(-14, -10)},
    {// queens
     S(-30, -1),  S(-14, 9),   S(-15, 29),  S(9, 14),    S(63, -11),
     S(41, -2),   S(40, -11),  S(26, 10),   S(-10, -20), S(-28, 10),
     S(-6, 24),   S(-16, 40),  S(-27, 65),  S(41, 5),    S(20, 5),
     S(40, -14),  S(-4, -19),  S(-9, -7),   S(0, 7),     S(-2, 37),
     S(37, 18),   S(59, 9),    S(54, -16),  S(41, -4),   S(-21, -3),
     S(-17, 14),  S(-15, 14),  S(-12, 36),  S(-5, 44),   S(10, 38),
     S(-1, 39),   S(0, 24),    S(-13, -1),  S(-19, 12),  S(-13, 11),
     S(-17, 38),  S(-9, 24),   S(-8, 20),   S(3, 15),    S(-4, 10),
     S(-18, -8),  S(-7, -8),   S(-10, 4),   S(-5, -16),  S(-3, -6),
     S(0, -10),   S(7, -18),   S(0, -15),   S(-24, -23), S(-10, -14),
     S(0, -10),   S(4, -46),   S(2, -30),   S(13, -41),  S(-1, -41),
     S(8, -55),   S(-7, -28),  S(-23, -12), S(-14, -16), S(-3, 4),
     S(-12, -18), S(-27, -19), S(-22, -27), S(-18, -57)},
    {// kings
     S(-15, -58), S(60, -33), S(75, -24), S(14, -9),  S(-64, 8),  S(-62, 25),
     S(59, 5),    S(29, -13), S(77, -22), S(8, 20),   S(-24, 20), S(57, 7),
     S(8, 20),    S(-20, 41), S(-1, 29),  S(-18, 16), S(-12, 9),  S(21, 18),
     S(48, 16),   S(2, 20),   S(5, 25),   S(59, 33),  S(72, 30),  S(1, 14),
     S(-7, 0),    S(-26, 27), S(-21, 29), S(-46, 35), S(-55, 37), S(-43, 39),
     S(-25, 32),  S(-61, 16), S(-84, 6),  S(-13, 7),  S(-47, 30), S(-89, 43),
     S(-92, 45),  S(-63, 35), S(-55, 24), S(-69, 9),  S(9, -12),  S(-12, 6),
     S(-42, 22),  S(-70, 33), S(-54, 33), S(-51, 27), S(-15, 13), S(-27, 3),
     S(37, -27),  S(17, -7),  S(-11, 10), S(-45, 19), S(-41, 21), S(-20, 13),
     S(17, -3),   S(25, -17), S(27, -54), S(52, -41), S(30, -24), S(-52, 0),
     S(4, -16),   S(-25, -4), S(34, -26), S(37, -49)}};
#undef S

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
  for (Piece pc = WhitePawn; pc < BlackPawn; pc++)
  {
      PieceType p = PieceToPieceType[pc];
      for (Square sq = SQ_A1; sq < NO_SQ; sq++)
      {
          PestoTable[pc][sq].mg = mg_value[p] + mg_pesto_table[p][sq^56];
          PestoTable[pc][sq].eg = eg_value[p] + eg_pesto_table[p][sq^56];
          PestoTable[pc + 6][sq].mg = mg_value[p] + mg_pesto_table[p][sq];
          PestoTable[pc + 6][sq].eg = eg_value[p] + eg_pesto_table[p][sq ];
      }
  }

  /*
  for (Piece pc = WhitePawn; pc < BlackPawn; pc++) {
    PieceType p = PieceToPieceType[pc];
    for (Square sq = SQ_A1; sq < NO_SQ; sq++) {
      PestoTable[pc][sq] = psqt[p][sq ^ 56];
      PestoTable[pc + 6][sq] = psqt[p][sq];
    }
  }
  */

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

  // PawnEntry *pe = pawnTable.probeEntry(board);

  // U64 white_occupancies = board.Us(White);
  // U64 black_occupancies = board.Us(Black);
  // U64 all = board.All();

  while (white_pieces) {
    Square sq = poplsb(white_pieces);
    Piece p = board.pieceAtB(sq);

    score[White] += PestoTable[p][sq];

    gamePhase += gamephaseInc[p];
    if (p == WhiteRook) {
      // Rook semi open file bonus
      if (IsSemiOpenFile(board, sq, White)) {
        score[White] += rook_semi_open_file;
      }
      // Rook open file bonus
      else if (IsOpenFile(board, sq)) {
        score[White] += rook_open_file;
      }
      // }else if (p == WhiteKing){
      //     U64 pawnsInFront = (board.piecesBB[WhitePawn] |
      //     board.piecesBB[BlackPawn]) & WhitePassedMasks[sq]; U64 ourPawns =
      //     pawnsInFront & white_occupancies & ~PawnAttacks()
    }
  }

  while (black_pieces) {
    Square sq = poplsb(black_pieces);
    Piece p = board.pieceAtB(sq);

    score[Black] += PestoTable[p][sq];

    gamePhase += gamephaseInc[p];
    if (p == BlackRook) {

      // Rook semi open file bonus
      if (IsSemiOpenFile(board, sq, Black)) {
        score[Black] += rook_semi_open_file;
      }
      // Rook open file bonus
      else if (IsOpenFile(board, sq)) {
        score[Black] += rook_open_file;
      }
    }
  }

  if (popcount(board.piecesBB[WhiteBishop]) > 1) {
    score[White] += bishop_pair_bonus;
  }
  if (popcount(board.piecesBB[BlackBishop]) > 1) {
    score[Black] += bishop_pair_bonus;
  }

  // tapered eval //
  int mgScore = (score[side2move].mg - score[otherSide].mg);
  int egScore = (score[side2move].eg - score[otherSide].eg);

  // mgScore += pe->value.mg;
  // egScore += pe->value.eg;

  int mgPhase = gamePhase;
  if (mgPhase > 24)
    mgPhase = 24;
  int egPhase = 24 - mgPhase;

  return (mgScore * mgPhase + egScore * egPhase) / 24;
}