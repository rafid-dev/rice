#pragma once

#include "chess.hpp"
#include "simd.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <numeric>
#include <vector>

/*
    Credits:
    Luecx (Author of Koivisto)
    Disservin (Author of Smallbrain)
*/

#define BUCKETS (4)
#define INPUT_SIZE (64 * 12 * BUCKETS)
#define HIDDEN_SIZE (768)
#define HIDDEN_DSIZE (HIDDEN_SIZE * 2)
#define OUTPUT_SIZE (1)

#define INPUT_QUANTIZATION (32)
#define HIDDEN_QUANTIZATON (128)

extern std::array<int16_t, INPUT_SIZE * HIDDEN_SIZE> inputWeights;
extern std::array<int16_t, HIDDEN_SIZE> inputBias;
extern std::array<int16_t, HIDDEN_SIZE * 2> hiddenWeights;
extern std::array<int32_t, OUTPUT_SIZE> hiddenBias;

namespace NNUE {

// clang-format off
constexpr int KING_BUCKET[64] {
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    2, 2, 3, 3, 3, 3, 2, 2,
    2, 2, 3, 3, 3, 3, 2, 2,
    2, 2, 3, 3, 3, 3, 2, 2,
    2, 2, 3, 3, 3, 3, 2, 2,
};
// clang-format on

inline int kingSquareIndex(Chess::Square kingSquare, Chess::Color kingColor) {
    kingSquare = Chess::Square((56 * kingColor) ^ kingSquare);
    return KING_BUCKET[kingSquare];
}

inline int index(Chess::PieceType pieceType, Chess::Color pieceColor, Chess::Square square, Chess::Color view,
                 Chess::Square kingSquare) {
    const int ksIndex = kingSquareIndex(kingSquare, view);
    square = Chess::Square(square ^ (56 * view));
    square = Chess::Square(square ^ (7 * !!(kingSquare & 0x4)));

    // clang-format off
    return square
           + pieceType * 64
           + !(pieceColor ^ view) * 64 * 6 + ksIndex * 64 * 6 * 2;
    // clang-format on
}

static inline int16_t relu(int16_t input) { return std::max(static_cast<int16_t>(0), input); }

struct Accumulator {
#if defined(USE_SIMD)
    alignas(ALIGNMENT) std::array<int16_t, HIDDEN_SIZE> white;
    alignas(ALIGNMENT) std::array<int16_t, HIDDEN_SIZE> black;
#else
    std::array<int16_t, HIDDEN_SIZE> white;
    std::array<int16_t, HIDDEN_SIZE> black;
#endif

    std::array<int16_t, HIDDEN_SIZE> &operator[](Chess::Color side) { return side == Chess::White ? white : black; }
    std::array<int16_t, HIDDEN_SIZE> &operator[](bool side) { return side ? black : white; }

    inline void copy(NNUE::Accumulator &acc) {
        std::copy(std::begin(acc.white), std::end(acc.white), std::begin(white));
        std::copy(std::begin(acc.black), std::end(acc.black), std::begin(black));
    }

    inline void clear() {
        std::copy(std::begin(inputBias), std::end(inputBias), std::begin(white));
        std::copy(std::begin(inputBias), std::end(inputBias), std::begin(black));
    }
};

struct Net {
    int32_t currentAccumulator = 0;

    std::array<Accumulator, 512> accumulator_stack;

    Net();

    inline void push() {
        accumulator_stack[currentAccumulator + 1].copy(accumulator_stack[currentAccumulator]);
        currentAccumulator++;
    }
    inline void pull() { 
        currentAccumulator--; 
    }
    inline void reset_accumulators() { 
        currentAccumulator = 0;
    }

    void refresh(Chess::Board &board);

    template <bool add>
    void updateAccumulator(Chess::PieceType pieceType, Chess::Color pieceColor, Chess::Square square,  Chess::Square kingSquare_White, Chess::Square kingSquare_Black);

    void updateAccumulator(Chess::PieceType pieceType, Chess::Color pieceColor, Chess::Square from_square,
                           Chess::Square to_square, Chess::Square kingSquare_White, Chess::Square kingSquare_Black);

    int32_t Evaluate(Chess::Color side);

    void Benchmark();

    void print_n_accumulator_inputs(const Accumulator &accumulator, size_t N) {
        for (size_t i = 0; i < N; i++) {
            std::cout << accumulator.white[i] << ", ";
        }

        std::cout << std::endl;

        for (size_t i = 0; i < N; i++) {
            std::cout << accumulator.black[i] << ", ";
        }

        std::cout << std::endl;
    }

    void print_indexes(const Chess::Board &board, const Chess::PieceType pt, const Chess::Square sq,
                       Chess::Square kingSquare) {
        std::cout << "W [" << int(pt) << ", " << int(board.colorOf(sq)) << ", " << int(sq)
                  << "  ]: " << index(pt, board.colorOf(sq), sq, Chess::White, kingSquare) << "\n";
        std::cout << "B [" << int(pt) << ", " << int(board.colorOf(sq)) << ", " << int(sq)
                  << "  ]: " << index(pt, board.colorOf(sq), sq, Chess::Black, kingSquare) << "\n";
    }
};

void Init(const std::string &file_name);

} // namespace NNUE