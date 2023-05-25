#include "nnue.h"

#include <algorithm>
#include <cstring>

#define INCBIN_STYLE INCBIN_STYLE_CAMEL
#include "incbin/incbin.h"
INCBIN(EVAL, "./epoch_90.net");

using namespace Chess;

#if defined(__AVX__) || defined(__AVX2__)
alignas(ALIGNMENT) std::array<int16_t, INPUT_SIZE * HIDDEN_SIZE> inputWeights;
alignas(ALIGNMENT) std::array<int16_t, HIDDEN_SIZE> inputBias;
alignas(ALIGNMENT) std::array<int16_t, HIDDEN_SIZE * 2> hiddenWeights;
alignas(ALIGNMENT) std::array<int32_t, OUTPUT_SIZE> hiddenBias;
#else
std::array<int16_t, INPUT_SIZE * HIDDEN_SIZE> inputWeights;
std::array<int16_t, HIDDEN_SIZE>              inputBias;
std::array<int16_t, HIDDEN_SIZE * 2>          hiddenWeights;
std::array<int32_t, OUTPUT_SIZE>              hiddenBias;
#endif

NNUE::Net::Net() { std::fill(accumulator_stack.begin(), accumulator_stack.end(), Accumulator()); }

void NNUE::Net::updateAccumulator(Chess::PieceType pieceType, Chess::Color pieceColor,
                                  Chess::Square from_square, Chess::Square to_square,
                                  Chess::Square kingSquare_White, Chess::Square kingSquare_Black) {

    Accumulator& accumulator = accumulator_stack[currentAccumulator];
#if defined(__AVX__) || defined(__AVX2__)
    for (auto side : {White, Black}) {
        const int            inputClear = index(pieceType, pieceColor, from_square, side,
                                     side == Chess::White ? kingSquare_White : kingSquare_Black);
        const int            inputAdd   = index(pieceType, pieceColor, to_square, side,
                                   side == Chess::White ? kingSquare_White : kingSquare_Black);

        avx_register_type_16 reg0;
        avx_register_type_16 reg1;
        avx_register_type_16 reg2;

        for (int i = 0; i < HIDDEN_SIZE; i += STRIDE_16_BIT) {
            reg0 = avx_load_reg((avx_register_type_16 const*) &accumulator[side][i]);
            reg1 = avx_load_reg((avx_register_type_16 const*) &inputWeights[inputClear * HIDDEN_SIZE + i]);
            reg2 = avx_load_reg((avx_register_type_16 const*) &inputWeights[inputAdd * HIDDEN_SIZE + i]);
            reg0 = avx_sub_epi16(reg0, reg1);
            reg0 = avx_add_epi16(reg0, reg2);

            avx_store_reg((avx_register_type_16*) &accumulator[side][i], reg0);
        }
    }
#else
    for (auto side : {White, Black}) {
                     const int inputClear = index(pieceType, pieceColor, from_square, side,
                                     side == Chess::White ? kingSquare_White : kingSquare_Black);
                     const int inputAdd   = index(pieceType, pieceColor, to_square, side,
                                   side == Chess::White ? kingSquare_White : kingSquare_Black);

                     for (int chunks = 0; chunks < HIDDEN_SIZE / 256; chunks++) {
                         const int offset = chunks * 256;
                         for (int i = offset; i < 256 + offset; i++) {
                             accumulator[side][i] += -inputWeights[inputClear * HIDDEN_SIZE + i]
                                                     + inputWeights[inputAdd * HIDDEN_SIZE + i];
            }
        }
    }
#endif
}

int32_t NNUE::Net::Evaluate(Color side) {
    Accumulator& accumulator = accumulator_stack[currentAccumulator];

#if defined(__AVX__) || defined(__AVX2__)

    avx_register_type_16 reluBias {};
    avx_register_type_32 res {};

    const auto           acc_act = (avx_register_type_16*) accumulator[side].data();
    const auto           acc_nac = (avx_register_type_16*) accumulator[!side].data();
    const auto           wgt     = (avx_register_type_16*) (hiddenWeights.data());

    for (int i = 0; i < HIDDEN_SIZE / STRIDE_16_BIT; i++) {
        res = avx_add_epi32(res, avx_madd_epi16(avx_max_epi16(acc_act[i], reluBias), wgt[i]));
    }

    for (int i = 0; i < HIDDEN_SIZE / STRIDE_16_BIT; i++) {
        res = avx_add_epi32(res, avx_madd_epi16(avx_max_epi16(acc_nac[i], reluBias),
                                                wgt[i + HIDDEN_SIZE / STRIDE_16_BIT]));
    }

    const auto outp = sumRegisterEpi32(res) + hiddenBias[0];
    return outp / INPUT_WEIGHT_MULTIPLIER / HIDDEN_WEIGHT_MULTIPLIER;

#else
    int32_t output = hiddenBias[0];

    for (int chunks = 0; chunks < HIDDEN_SIZE / 256; chunks++) {
                     const int offset = chunks * 256;
                     for (int i = 0; i < 256; i++) {
                         output += relu(accumulator[side][i + offset]) * hiddenWeights[i + offset];
        }
    }

    for (int chunks = 0; chunks < HIDDEN_SIZE / 256; chunks++) {
                     const int offset = chunks * 256;
                     for (int i = 0; i < 256; i++) {
                         output += relu(accumulator[!side][i + offset]) * hiddenWeights[HIDDEN_SIZE + i + offset];
        }
    }

    return output / INPUT_WEIGHT_MULTIPLIER / HIDDEN_WEIGHT_MULTIPLIER;

#endif
}

void ReadBin() {
    uint64_t memoryIndex = 0;

    std::memcpy(inputWeights.data(), &gEVALData[memoryIndex],
                INPUT_SIZE * HIDDEN_SIZE * sizeof(int16_t));
    memoryIndex += INPUT_SIZE * HIDDEN_SIZE * sizeof(int16_t);

    std::memcpy(inputBias.data(), &gEVALData[memoryIndex], HIDDEN_SIZE * sizeof(int16_t));
    memoryIndex += HIDDEN_SIZE * sizeof(int16_t);

    std::memcpy(hiddenWeights.data(), &gEVALData[memoryIndex],
                HIDDEN_DSIZE * OUTPUT_SIZE * sizeof(int16_t));
    memoryIndex += HIDDEN_DSIZE * OUTPUT_SIZE * sizeof(int16_t);

    std::memcpy(hiddenBias.data(), &gEVALData[memoryIndex], OUTPUT_SIZE * sizeof(int32_t));
    memoryIndex += OUTPUT_SIZE * sizeof(int32_t);

#ifdef DEBUG
    std::cout << "Memory index: " << memoryIndex << std::endl;
    std::cout << "Size: " << gEVALSize << std::endl;
    std::cout << "Bias: " << hiddenBias[0] / INPUT_WEIGHT_MULTIPLIER / HIDDEN_WEIGHT_MULTIPLIER
              << std::endl;

    std::cout << std::endl;
#endif
}

void NNUE::Init(const std::string& file_name) { ReadBin(); }