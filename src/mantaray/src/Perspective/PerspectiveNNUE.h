//
// Copyright (c) 2023 MantaRay authors. See the list of authors for more details.
// Licensed under MIT.
//

#ifndef MANTARAY_PERSPECTIVENNUE_H
#define MANTARAY_PERSPECTIVENNUE_H

#include <array>
#include <cstdint>
#include <cassert>

#include "PerspectiveAccumulator.h"
#include "../SIMD.h"
#include "../AccumulatorOperation.h"
#include "../IO/BinaryFileStream.h"
#include "../IO/BinaryMemoryStream.h"
#include "../IO/MarlinflowStream.h"

namespace MantaRay
{

    template<typename T, typename OT, typename Activation,
            uint16_t InputSize, uint16_t HiddenSize, uint16_t OutputSize,
            uint16_t AccumulatorStackSize, T Scale, T QuantizationFeature, T QuantizationOutput>
    class PerspectiveNetwork
    {

        // Support more types in the future, but currently disable all others.
        static_assert(std::is_same_v<T, int16_t> && std::is_same_v<OT, int32_t>,
                "This type is currently not supported.");

        // Support less sizes in the future, but currently disable.
        static_assert(InputSize > 32 && HiddenSize > 32, "This network size is currently not supported.");

        // The accumulator stack size should not be 0 or less.
        static_assert(AccumulatorStackSize > 0, "The accumulator stack size must at least be greater than zero.");

        // The constants used should make sense. Change this later if requested.
        static_assert(Scale > 0 && QuantizationFeature > 127 && QuantizationOutput > 31,
                "These scale and quantization constants don't seem right.");

        private:
            constexpr static uint16_t ColorStride = 64 * 6;
            constexpr static uint8_t  PieceStride = 64    ;

#ifdef __AVX512BW__
            alignas(64) std::array<T , InputSize * HiddenSize     > FeatureWeight;
            alignas(64) std::array<T , HiddenSize                 > FeatureBias  ;
            alignas(64) std::array<T , HiddenSize * 2 * OutputSize> OutputWeight ;
            alignas(64) std::array<T , OutputSize                 > OutputBias   ;
            alignas(64) std::array<OT, OutputSize                 > Output       ;
#elifdef __AVX2__
            alignas(32) std::array<T , InputSize * HiddenSize     > FeatureWeight;
            alignas(32) std::array<T , HiddenSize                 > FeatureBias  ;
            alignas(32) std::array<T , HiddenSize * 2 * OutputSize> OutputWeight ;
            alignas(32) std::array<T , OutputSize                 > OutputBias   ;
            alignas(32) std::array<OT, OutputSize                 > Output       ;
#else
            std::array<T , InputSize * HiddenSize     > FeatureWeight;
            std::array<T , HiddenSize                 > FeatureBias  ;
            std::array<T , HiddenSize * 2 * OutputSize> OutputWeight ;
            std::array<T , OutputSize                 > OutputBias   ;
            std::array<OT, OutputSize                 > Output       ;
#endif

            std::array<PerspectiveAccumulator<T, HiddenSize>, AccumulatorStackSize> Accumulators;
            uint16_t CurrentAccumulator = 0;

            void InitializeAccumulatorStack()
            {
                PerspectiveAccumulator<T, HiddenSize> accumulator;
                std::fill(std::begin(Accumulators), std::end(Accumulators), accumulator);
            }

        public:
            __attribute__((unused)) PerspectiveNetwork()
            {
                InitializeAccumulatorStack();
            }

            __attribute__((unused)) explicit PerspectiveNetwork(BinaryFileStream &stream)
            {
                InitializeAccumulatorStack();

                stream.ReadArray(FeatureWeight);
                stream.ReadArray(FeatureBias  );
                stream.ReadArray(OutputWeight );
                stream.ReadArray(OutputBias   );
            }

            __attribute__((unused)) explicit PerspectiveNetwork(BinaryMemoryStream &stream)
            {
                InitializeAccumulatorStack();

                stream.ReadArray(FeatureWeight);
                stream.ReadArray(FeatureBias  );
                stream.ReadArray(OutputWeight );
                stream.ReadArray(OutputBias   );
            }

            __attribute__((unused)) explicit PerspectiveNetwork(MarlinflowStream &stream)
            {
                InitializeAccumulatorStack();

                stream.Read2DArray("ft.weight" , FeatureWeight, HiddenSize    , QuantizationFeature,
                                   true );
                stream.Read2DArray("out.weight", OutputWeight , HiddenSize * 2, QuantizationOutput ,
                                   false);

                stream.ReadArray("ft.bias" , FeatureBias, QuantizationFeature                     );
                stream.ReadArray("out.bias", OutputBias , QuantizationFeature * QuantizationOutput);
            }

            __attribute__((unused)) void WriteTo(BinaryFileStream &stream)
            {
                stream.WriteMode();

                stream.WriteArray(FeatureWeight);
                stream.WriteArray(FeatureBias  );
                stream.WriteArray(OutputWeight );
                stream.WriteArray(OutputBias   );
            }

            __attribute__((unused)) inline void ResetAccumulator()
            {
                CurrentAccumulator = 0;
            }

            __attribute__((unused)) inline void PushAccumulator()
            {
                Accumulators[CurrentAccumulator].CopyTo(Accumulators[++CurrentAccumulator]);

                assert(CurrentAccumulator < AccumulatorStackSize);
            }

            __attribute__((unused)) inline void PullAccumulator()
            {
                assert(CurrentAccumulator > 0);

                CurrentAccumulator--;
            }

            __attribute__((unused)) inline void RefreshAccumulator()
            {
                PerspectiveAccumulator<T, HiddenSize>& accumulator = Accumulators[CurrentAccumulator];
                accumulator.Zero();
                accumulator.LoadBias(FeatureBias);
            }

            __attribute__((unused)) inline void EfficientlyUpdateAccumulator(const uint8_t piece, const uint8_t color,
                                                                             const uint8_t from, const uint8_t to)
            {
                const uint16_t pieceStride = piece * PieceStride;

                const uint32_t whiteIndexFrom =  color       * ColorStride + pieceStride +        from;
                const uint32_t blackIndexFrom = (color ^ 1)  * ColorStride + pieceStride + (from ^ 56);
                const uint32_t whiteIndexTo   =  color       * ColorStride + pieceStride +          to;
                const uint32_t blackIndexTo   = (color ^ 1)  * ColorStride + pieceStride +   (to ^ 56);

                PerspectiveAccumulator<T, HiddenSize>& accumulator = Accumulators[CurrentAccumulator];

                SIMD::SubtractAndAddToAll(accumulator.White, accumulator.Black,
                                          FeatureWeight,
                                          whiteIndexFrom * HiddenSize,
                                          whiteIndexTo   * HiddenSize,
                                          blackIndexFrom * HiddenSize,
                                          blackIndexTo   * HiddenSize);
            }

            template<AccumulatorOperation Operation>
            __attribute__((unused)) inline void EfficientlyUpdateAccumulator(const uint8_t piece, const uint8_t color,
                                                                             const uint8_t sq)
            {
                const uint16_t pieceStride = piece * PieceStride;

                const uint32_t whiteIndex =  color      * ColorStride + pieceStride +        sq;
                const uint32_t blackIndex = (color ^ 1) * ColorStride + pieceStride + (sq ^ 56);

                PerspectiveAccumulator<T, HiddenSize>& accumulator = Accumulators[CurrentAccumulator];

                if (Operation == AccumulatorOperation::Activate)
                    SIMD::AddToAll(accumulator.White,
                                   accumulator.Black,
                                   FeatureWeight,
                                   whiteIndex * HiddenSize,
                                   blackIndex * HiddenSize);

                else SIMD::SubtractFromAll(accumulator.White,
                                           accumulator.Black,
                                           FeatureWeight,
                                           whiteIndex * HiddenSize,
                                           blackIndex * HiddenSize);
            }

            __attribute__((unused)) inline int32_t Evaluate(const uint8_t colorToMove)
            {
                PerspectiveAccumulator<T, HiddenSize>& accumulator = Accumulators[CurrentAccumulator];

                if (colorToMove == 0) SIMD::ActivateFlattenAndForward<Activation>(
                        accumulator.White,
                        accumulator.Black,
                        OutputWeight,
                        OutputBias,
                        Output,
                        0);

                else SIMD::ActivateFlattenAndForward<Activation>(
                        accumulator.Black,
                        accumulator.White,
                        OutputWeight,
                        OutputBias,
                        Output,
                        0);

                return Output[0] * Scale / (QuantizationFeature * QuantizationOutput);
            }

    };

} // MantaRay

#endif //MANTARAY_PERSPECTIVENNUE_H
