#include "eval.h"
#include "psq.h"

#define SETBIT(bitboard, square) ((bitboard) |= (1ULL << (square)))

// rook open file bonus
Score rook_semi_open_file(8, 7);
Score rook_open_file(16, 14);

// king safety
Score king_semi_open_file(0, 0);
Score king_open_file(0, 0);

// bishop pair
Score bishop_pair_bonus(30, 34);

U64 FileMasks[64];
U64 RankMasks[64];
U64 IsolatedMasks[64];
U64 WhitePassedMasks[64];
U64 BlackPassedMasks[64];

int gamephaseInc[12] = {0, 0, 1, 1, 1, 1, 2, 2, 4, 4, 0, 0};
// int mg_value[6] = {82, 337, 365, 477, 1025, 0};
// int eg_value[6] = {94, 281, 297, 512, 936, 0};

int mg_value[6] = {80, 335, 364, 475, 1025, 0};
int eg_value[6] = { 91, 280, 296, 509, 940, 0};

const int piece_values[12] = {100, 300, 300, 500, 900, 0};

Score PestoTable[12][64];

static U64 SetFileRankMask(int file_number, int rank_number)
{
    U64 mask = 0ULL;

    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;

            if (file_number != -1)
            {
                if (file == file_number)
                    mask |= SETBIT(mask, square);
            }

            else if (rank_number != -1)
            {
                if (rank == rank_number)
                    mask |= SETBIT(mask, square);
            }
        }
    }

    return mask;
}

void InitEvaluationMasks()
{

    // Init files and ranks masks
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;

            FileMasks[square] |= SetFileRankMask(file, -1);
        }
    }
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;

            RankMasks[square] |= SetFileRankMask(-1, rank);
        }
    }

    // Init isolated pawn masks
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;
            IsolatedMasks[square] |= SetFileRankMask(file - 1, -1);
            IsolatedMasks[square] |= SetFileRankMask(file + 1, -1);
        }
    }

    // Init passed pawn masks for White
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;
            WhitePassedMasks[square] |= SetFileRankMask(file - 1, -1);
            WhitePassedMasks[square] |= SetFileRankMask(file, -1);
            WhitePassedMasks[square] |= SetFileRankMask(file + 1, -1);

            for (int i = 0; i < (8 - rank); i++)
            {
                WhitePassedMasks[square] &= ~RankMasks[(7 - i) * 8 + file];
            }
        }
    }

    // Init passed pawn masks for Black
    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;
            BlackPassedMasks[square] |= SetFileRankMask(file - 1, -1);
            BlackPassedMasks[square] |= SetFileRankMask(file, -1);
            BlackPassedMasks[square] |= SetFileRankMask(file + 1, -1);

            for (int i = 0; i < rank + 1; i++)
            {
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

void InitPestoTables()
{
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

    // std::cout << "constexpr int32_t PsqTable[12][64] = {";
    // for (Piece i = WhitePawn; i < BlackPawn; i++){
    //     std::cout << "\n{\n";
    //     for (int rank = 0; rank < 8; rank++){
    //         for (int file = 0; file < 8; file++){
    //             int sq = rank * 8 + file;
    //             std::cout << "S(" << PestoTable[i][sq].mg << "," << PestoTable[i][sq].eg << "),";
    //         }
    //         std::cout << "\n";
    //     }
    //     std::cout << "},\n";
    // }
    // std::cout<< "},\n";
}

// Check if a file is semi open for a given side
static inline bool IsSemiOpenFile(Board &board, Square sq, Color us)
{
    return (((us == White) ? board.piecesBB[WhitePawn] : board.piecesBB[BlackPawn]) & FileMasks[sq]) == 0;
}

// Check if a file is open
static inline bool IsOpenFile(Board &board, Square sq)
{
    return ((board.piecesBB[WhitePawn] | board.piecesBB[BlackPawn]) & FileMasks[sq]) == 0;
}

int Evaluate(Board &board, PawnTable& pawnTable)
{
    Score score[2];

    int gamePhase = 0;

    Color side2move = board.sideToMove;
    Color otherSide = ~board.sideToMove;

    U64 white_pieces = board.Us(White);
    U64 black_pieces = board.Us(Black);

    //PawnEntry *pe = pawnTable.probeEntry(board);

    // U64 white_occupancies = board.Us(White);
    // U64 black_occupancies = board.Us(Black);
    // U64 all = board.All();

    while (white_pieces)
    {
        Square sq = poplsb(white_pieces);
        Piece p = board.pieceAtB(sq);

        score[White] += PestoTable[p][sq];

        gamePhase += gamephaseInc[p];
        if (p == WhiteRook)
        {
            // Rook semi open file bonus
            if (IsSemiOpenFile(board, sq, White))
            {
                score[White] += rook_semi_open_file;
            }
            // Rook open file bonus
            else if (IsOpenFile(board, sq))
            {
                score[White] += rook_open_file;
            }
        }
    }

    while (black_pieces)
    {
        Square sq = poplsb(black_pieces);
        Piece p = board.pieceAtB(sq);

        score[Black] += PestoTable[p][sq];

        gamePhase += gamephaseInc[p];
        if (p == BlackRook)
        {

            // Rook semi open file bonus
            if (IsSemiOpenFile(board, sq, Black))
            {
                score[Black] += rook_semi_open_file;
            }
            // Rook open file bonus
            else if (IsOpenFile(board, sq))
            {
                score[Black] += rook_open_file;
            }
        }
    }

    //Bishop pair bonus
    // if (popcount(board.piecesBB[WhiteBishop]) > 1)
    // {
    //     score[White] += bishop_pair_bonus;
    // }
    // if (popcount(board.piecesBB[BlackBishop]) > 1)
    // {
    //     score[Black] += bishop_pair_bonus;
    // }

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

int Evaluate(Board &board)
{
    Score score[2];

    int gamePhase = 0;

    Color side2move = board.sideToMove;
    Color otherSide = ~board.sideToMove;

    U64 white_pieces = board.Us(White);
    U64 black_pieces = board.Us(Black);

    //PawnEntry *pe = pawnTable.probeEntry(board);

    // U64 white_occupancies = board.Us(White);
    // U64 black_occupancies = board.Us(Black);
    // U64 all = board.All();

    while (white_pieces)
    {
        Square sq = poplsb(white_pieces);
        Piece p = board.pieceAtB(sq);

        score[White] += PestoTable[p][sq];

        gamePhase += gamephaseInc[p];
        if (p == WhiteRook)
        {
            // Rook semi open file bonus
            if (IsSemiOpenFile(board, sq, White))
            {
                score[White] += rook_semi_open_file;
            }
            // Rook open file bonus
            else if (IsOpenFile(board, sq))
            {
                score[White] += rook_open_file;
            }
        }
    }

    while (black_pieces)
    {
        Square sq = poplsb(black_pieces);
        Piece p = board.pieceAtB(sq);

        score[Black] += PestoTable[p][sq];

        gamePhase += gamephaseInc[p];
        if (p == BlackRook)
        {

            // Rook semi open file bonus
            if (IsSemiOpenFile(board, sq, Black))
            {
                score[Black] += rook_semi_open_file;
            }
            // Rook open file bonus
            else if (IsOpenFile(board, sq))
            {
                score[Black] += rook_open_file;
            }
        }
    }

    //Bishop pair bonus
    // if (popcount(board.piecesBB[WhiteBishop]) > 1)
    // {
    //     score[White] += bishop_pair_bonus;
    // }
    // if (popcount(board.piecesBB[BlackBishop]) > 1)
    // {
    //     score[Black] += bishop_pair_bonus;
    // }

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

