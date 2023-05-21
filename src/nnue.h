#pragma once

#include "chess.hpp"
#include "simd.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <numeric>
#include <vector>

#define INPUT_SIZE               (64 * 12)
#define HIDDEN_SIZE              (512)
#define HIDDEN_DSIZE             (HIDDEN_SIZE * 2)
#define OUTPUT_SIZE              (1)

#define INPUT_WEIGHT_MULTIPLIER  (32)
#define HIDDEN_WEIGHT_MULTIPLIER (128)

extern std::array<int16_t, INPUT_SIZE * HIDDEN_SIZE> inputWeights;
extern std::array<int16_t, HIDDEN_SIZE>              inputBias;
extern std::array<int16_t, HIDDEN_SIZE * 2>          hiddenWeights;
extern std::array<int32_t, OUTPUT_SIZE>              hiddenBias;

// Credits to Luecx and Disservin
namespace NNUE {

[[nodiscard]] inline int index(Chess::PieceType pieceType, Chess::Color pieceColor,
                               Chess::Square square, Chess::Color view, Chess::Square kingSquare) {

    square = Chess::Square(square ^ (56 * view));
    square = Chess::Square(square ^ (7 * !!(kingSquare & 0x4)));

    // clang-format off
    return square
           + pieceType * 64
           + !(pieceColor ^ view) * 64 * 6;
    // clang-format on
}

static inline int16_t relu(int16_t input) { return std::max(static_cast<int16_t>(0), input); }

struct Accumulator {
#if defined(__AVX512F__) || defined(__AVX__) || defined(__AVX2__)
    alignas(ALIGNMENT) std::array<int16_t, HIDDEN_SIZE> white;
    alignas(ALIGNMENT) std::array<int16_t, HIDDEN_SIZE> black;
#else
    std::array<int16_t, HIDDEN_SIZE> white;
    std::array<int16_t, HIDDEN_SIZE> black;
#endif

    inline void copy(const NNUE::Accumulator& acc) {
        std::copy(std::begin(acc.white), std::end(acc.white), std::begin(white));
        std::copy(std::begin(acc.black), std::end(acc.black), std::begin(black));
    }

    std::array<int16_t, HIDDEN_SIZE>& operator[](Chess::Color side) {
        return side == Chess::White ? white : black;
    }
    std::array<int16_t, HIDDEN_SIZE>& operator[](bool side) { return side ? black : white; }

    inline void                       clear() {
                              std::copy(std::begin(inputBias), std::end(inputBias), std::begin(white));
                              std::copy(std::begin(inputBias), std::end(inputBias), std::begin(black));
    }
};

struct Net {
    int32_t                      currentAccumulator = 0;

    std::array<Accumulator, 512> accumulator_stack;

    Net();

    inline void push() {
        accumulator_stack[currentAccumulator + 1].copy(accumulator_stack[currentAccumulator]);
        currentAccumulator++;
    }

    inline void pull() { currentAccumulator--; }

    inline void reset_accumulators() { currentAccumulator = 0; }

    void        refresh(Chess::Board& board) {

               Accumulator& accumulator = accumulator_stack[currentAccumulator];
               accumulator.clear();

               U64                 pieces           = board.All();

               const Chess::Square kingSquare_White = board.KingSQ(Chess::White);
               const Chess::Square kingSquare_Black = board.KingSQ(Chess::Black);

               while (pieces) {
                   Chess::Square    sq = Chess::poplsb(pieces);
                   Chess::PieceType pt = board.pieceTypeAtB(sq);

                   updateAccumulator<true>(pt, board.colorOf(sq), sq, kingSquare_White, kingSquare_Black);
        }
    }

    template<bool add>
    void updateAccumulator(Chess::PieceType pieceType, Chess::Color pieceColor, Chess::Square square,
                           Chess::Square kingSquare_White, Chess::Square kingSquare_Black) {

        Accumulator& accumulator = accumulator_stack[currentAccumulator];

#if defined(__AVX512F__) || defined(__AVX__) || defined(__AVX2__)
        if constexpr (add) {

            for (auto side : {Chess::White, Chess::Black}) {
                const int            input = index(pieceType, pieceColor, square, side,
                                        side == Chess::White ? kingSquare_White : kingSquare_Black);
                avx_register_type_16 reg0;
                avx_register_type_16 reg1;

                for (int i = 0; i < HIDDEN_SIZE; i += STRIDE_16_BIT) {
                    reg0 = avx_load_reg((avx_register_type_16 const*) &accumulator[side][i]);
                    reg1 = avx_load_reg(
                        (avx_register_type_16 const*) &inputWeights[input * HIDDEN_SIZE + i]);
                    reg0 = avx_add_epi16(reg0, reg1);
                    avx_store_reg((avx_register_type_16*) &accumulator[side][i], reg0);
                }
            }
        } else {
            for (auto side : {Chess::White, Chess::Black}) {
                const int            input = index(pieceType, pieceColor, square, side,
                                        side == Chess::White ? kingSquare_White : kingSquare_Black);
                                        
                avx_register_type_16 reg0;
                avx_register_type_16 reg1;

                for (int i = 0; i < HIDDEN_SIZE; i += STRIDE_16_BIT) {
                    reg0 = avx_load_reg((avx_register_type_16 const*) &accumulator[side][i]);
                    reg1 = avx_load_reg(
                        (avx_register_type_16 const*) &inputWeights[input * HIDDEN_SIZE + i]);
                    reg0 = avx_sub_epi16(reg0, reg1);
                    avx_store_reg((avx_register_type_16*) &accumulator[side][i], reg0);
                }
            }
        }
#else
        if constexpr (add) {

            for (auto side : {Chess::White, Chess::Black}) {
                const int input = index(pieceType, pieceColor, square, side,
                                        side == Chess::White ? kingSquare_White : kingSquare_Black);
                for (int chunks = 0; chunks < HIDDEN_SIZE / 256; chunks++) {
                    const int offset = chunks * 256;
                    for (int i = offset; i < 256 + offset; i++) {
                        accumulator[side][i] += inputWeights[input * HIDDEN_SIZE + i];
                    }
                }
            }
        } else {
           for (auto side : {Chess::White, Chess::Black}) {
                const int input = index(pieceType, pieceColor, square, side,
                                        side == Chess::White ? kingSquare_White : kingSquare_Black);
                for (int chunks = 0; chunks < HIDDEN_SIZE / 256; chunks++) {
                    const int offset = chunks * 256;
                    for (int i = offset; i < 256 + offset; i++) {
                        accumulator[side][i] -= inputWeights[input * HIDDEN_SIZE + i];
                    }
                }
        }
#endif
    }

    void    updateAccumulator(Chess::PieceType pieceType, Chess::Color pieceColor,
                              Chess::Square from_square, Chess::Square to_square,
                              Chess::Square kingSquare_White, Chess::Square kingSquare_Black);

    int32_t Evaluate(Chess::Color side);
    void    print_n_accumulator_inputs(const Accumulator& accumulator, size_t N) {
           for (size_t i = 0; i < N; i++) {
               std::cout << accumulator.white[i] << ", ";
        }

           std::cout << std::endl;

           for (size_t i = 0; i < N; i++) {
               std::cout << accumulator.black[i] << ", ";
        }

           std::cout << std::endl;
    }

    void print_indexes(const Chess::Board& board, const Chess::PieceType pt, const Chess::Square sq,
                       Chess::Square kingSquare) {
        std::cout << "W [" << int(pt) << ", " << int(board.colorOf(sq)) << ", " << int(sq)
                  << "  ]: " << index(pt, board.colorOf(sq), sq, Chess::White, kingSquare) << "\n";
        std::cout << "B [" << int(pt) << ", " << int(board.colorOf(sq)) << ", " << int(sq)
                  << "  ]: " << index(pt, board.colorOf(sq), sq, Chess::Black, kingSquare) << "\n";
    }
};

void Init(const std::string& file_name);

}    // namespace NNUE