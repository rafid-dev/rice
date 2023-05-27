#pragma once

#include "chess.hpp"
#include "simd.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <numeric>
#include <vector>

#define BUCKETS (4)
#define INPUT_SIZE (64 * 12 * BUCKETS)
#define HIDDEN_SIZE (768)
#define HIDDEN_DSIZE (HIDDEN_SIZE * 2)
#define OUTPUT_SIZE (1)

#define INPUT_WEIGHT_MULTIPLIER (32)
#define HIDDEN_WEIGHT_MULTIPLIER (128)

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

// Credits to Luecx and Disservin

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
#if defined(__AVX__) || defined(__AVX2__)
    alignas(ALIGNMENT) std::array<int16_t, HIDDEN_SIZE> white;
    alignas(ALIGNMENT) std::array<int16_t, HIDDEN_SIZE> black;
#else
    std::array<int16_t, HIDDEN_SIZE> white;
    std::array<int16_t, HIDDEN_SIZE> black;
#endif
    std::array<int16_t, HIDDEN_SIZE> &operator[](Chess::Color side) { return side == Chess::White ? white : black; }
    std::array<int16_t, HIDDEN_SIZE> &operator[](bool side) { return side ? black : white; }

    inline void copy(NNUE::Accumulator &acc) {
#if defined(__AVX__) || defined(__AVX2__)
        for (auto side : {Chess::White, Chess::Black}) {
            const auto inp = reinterpret_cast<avx_register_type_16 *>(acc[side].data());
            const auto out = reinterpret_cast<avx_register_type_16 *>(this->operator[](side).data());

            for (int i = 0; i < HIDDEN_SIZE / (STRIDE_16_BIT * 4); i++) {
                const int baseIdx = i * 4;

                out[baseIdx] = inp[baseIdx];
                out[baseIdx + 1] = inp[baseIdx + 1];
                out[baseIdx + 2] = inp[baseIdx + 2];
                out[baseIdx + 3] = inp[baseIdx + 3];
            }
        }
#else
        std::copy(std::begin(acc.white), std::end(acc.white), std::begin(white));
        std::copy(std::begin(acc.black), std::end(acc.black), std::begin(black));
#endif
    }

    inline void clear() {
#if defined(__AVX__) || defined(__AVX2__)
        for (auto side : {Chess::White, Chess::Black}) {
            const auto wgt = reinterpret_cast<avx_register_type_16 *>(inputBias.data());
            const auto out = reinterpret_cast<avx_register_type_16 *>(this->operator[](side).data());

            for (int i = 0; i < HIDDEN_SIZE / (STRIDE_16_BIT * 4); i++) {
                const int baseIdx = i * 4;

                out[baseIdx] = wgt[baseIdx];
                out[baseIdx + 1] = wgt[baseIdx + 1];
                out[baseIdx + 2] = wgt[baseIdx + 2];
                out[baseIdx + 3] = wgt[baseIdx + 3];
            }
        }
#else
        std::copy(std::begin(inputBias), std::end(inputBias), std::begin(white));
        std::copy(std::begin(inputBias), std::end(inputBias), std::begin(black));
#endif
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

    inline void pull() { currentAccumulator--; }

    inline void reset_accumulators() { currentAccumulator = 0; }

    void refresh(Chess::Board &board) {

        Accumulator &accumulator = accumulator_stack[currentAccumulator];
        accumulator.clear();

        U64 pieces = board.All();

        const Chess::Square kingSquare_White = board.KingSQ(Chess::White);
        const Chess::Square kingSquare_Black = board.KingSQ(Chess::Black);

        while (pieces) {
            Chess::Square sq = Chess::poplsb(pieces);
            Chess::PieceType pt = board.pieceTypeAtB(sq);

            updateAccumulator<true>(pt, board.colorOf(sq), sq, kingSquare_White, kingSquare_Black);
        }
    }

    template <bool add>
    void updateAccumulator(Chess::PieceType pieceType, Chess::Color pieceColor, Chess::Square square,
                           Chess::Square kingSquare_White, Chess::Square kingSquare_Black) {

        Accumulator &accumulator = accumulator_stack[currentAccumulator];

#if defined(__AVX__) || defined(__AVX2__)

        for (auto side : {Chess::White, Chess::Black}) {
            const int input =
                index(pieceType, pieceColor, square, side, side == Chess::White ? kingSquare_White : kingSquare_Black);

            const auto wgt = reinterpret_cast<avx_register_type_16 *>(inputWeights.data());
            const auto inp = reinterpret_cast<avx_register_type_16 *>(accumulator[side].data());
            const auto out = reinterpret_cast<avx_register_type_16 *>(accumulator[side].data());

            constexpr int blockSize = 4; // Adjust the block size for optimal cache usage

            if constexpr (add) {
                for (int block = 0; block < HIDDEN_SIZE / (STRIDE_16_BIT * blockSize); block++) {
                    const int baseIdx = (input * HIDDEN_SIZE / STRIDE_16_BIT) + (block * blockSize);

                    avx_register_type_16 *outPtr = out + (block * blockSize);
                    const avx_register_type_16 *wgtPtr = wgt + baseIdx;
                    const avx_register_type_16 *inpPtr = inp + (block * blockSize);

                    avx_register_type_16 sum0 = avx_add_epi16(inpPtr[0], wgtPtr[0]);
                    avx_register_type_16 sum1 = avx_add_epi16(inpPtr[1], wgtPtr[1]);
                    avx_register_type_16 sum2 = avx_add_epi16(inpPtr[2], wgtPtr[2]);
                    avx_register_type_16 sum3 = avx_add_epi16(inpPtr[3], wgtPtr[3]);

                    for (int i = 4; i < blockSize; i++) {
                        sum0 = avx_add_epi16(sum0, wgtPtr[i]);
                        sum1 = avx_add_epi16(sum1, wgtPtr[i + blockSize]);
                        sum2 = avx_add_epi16(sum2, wgtPtr[i + blockSize * 2]);
                        sum3 = avx_add_epi16(sum3, wgtPtr[i + blockSize * 3]);
                    }

                    outPtr[0] = sum0;
                    outPtr[1] = sum1;
                    outPtr[2] = sum2;
                    outPtr[3] = sum3;
                }
            } else {
                for (int block = 0; block < HIDDEN_SIZE / (STRIDE_16_BIT * blockSize); block++) {
                    const int baseIdx = (input * HIDDEN_SIZE / STRIDE_16_BIT) + (block * blockSize);

                    avx_register_type_16 *outPtr = out + (block * blockSize);
                    const avx_register_type_16 *wgtPtr = wgt + baseIdx;
                    const avx_register_type_16 *inpPtr = inp + (block * blockSize);

                    avx_register_type_16 diff0 = avx_sub_epi16(inpPtr[0], wgtPtr[0]);
                    avx_register_type_16 diff1 = avx_sub_epi16(inpPtr[1], wgtPtr[1]);
                    avx_register_type_16 diff2 = avx_sub_epi16(inpPtr[2], wgtPtr[2]);
                    avx_register_type_16 diff3 = avx_sub_epi16(inpPtr[3], wgtPtr[3]);

                    for (int i = 4; i < blockSize; i++) {
                        diff0 = avx_sub_epi16(diff0, wgtPtr[i]);
                        diff1 = avx_sub_epi16(diff1, wgtPtr[i + blockSize]);
                        diff2 = avx_sub_epi16(diff2, wgtPtr[i + blockSize * 2]);
                        diff3 = avx_sub_epi16(diff3, wgtPtr[i + blockSize * 3]);
                    }

                    outPtr[0] = diff0;
                    outPtr[1] = diff1;
                    outPtr[2] = diff2;
                    outPtr[3] = diff3;
                }
            }
        }
#else
        for (auto side : {Chess::White, Chess::Black}) {
            const int input =
                index(pieceType, pieceColor, square, side, side == Chess::White ? kingSquare_White : kingSquare_Black);
            for (int chunks = 0; chunks < HIDDEN_SIZE / 256; chunks++) {
                const int offset = chunks * 256;
                for (int i = offset; i < 256 + offset; i++) {
                    if constexpr (add) {
                        accumulator[side][i] += inputWeights[input * HIDDEN_SIZE + i];
                    } else {
                        accumulator[side][i] -= inputWeights[input * HIDDEN_SIZE + i];
                    }
                }
            }
        }
#endif
    }

    void updateAccumulator(Chess::PieceType pieceType, Chess::Color pieceColor, Chess::Square from_square,
                           Chess::Square to_square, Chess::Square kingSquare_White, Chess::Square kingSquare_Black);

    int32_t Evaluate(Chess::Color side);
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