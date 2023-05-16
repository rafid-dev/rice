//
// Copyright (c) 2023 MantaRay authors. See the list of authors for more details.
// Licensed under MIT.
//

#ifndef MANTARAY_PERSPECTIVEACCUMULATOR_H
#define MANTARAY_PERSPECTIVEACCUMULATOR_H

#include <array>

#ifdef __AVX512BW__
#include "../Backend/Avx512.h"
#elifdef __AVX2__
#include "../Backend/Avx2.h"
#endif

namespace MantaRay
{

    template<typename T, size_t AccumulatorSize>
    class PerspectiveAccumulator
    {

        public:
#ifdef __AVX512BW__
            alignas(64) std::array<T, AccumulatorSize> White;
            alignas(64) std::array<T, AccumulatorSize> Black;
#elifdef __AVX2__
            alignas(32) std::array<T, AccumulatorSize> White;
            alignas(32) std::array<T, AccumulatorSize> Black;
#else
            std::array<T, AccumulatorSize> White;
            std::array<T, AccumulatorSize> Black;
#endif

            PerspectiveAccumulator()
            {
                std::fill(std::begin(White), std::end(White), 0);
                std::fill(std::begin(Black), std::end(Black), 0);
            }

            inline void CopyTo(PerspectiveAccumulator<T, AccumulatorSize>& accumulator)
            {
                // Certain instructions can be limited further down, but due to alignment issues, performance may not be
                // best. Thus, currently limiting to peak instruction set.

#ifdef __AVX512BW__ // Limit this to AVX512F instead.
                Vec512I zmm0;

                for (size_t i = 0; i < AccumulatorSize; i += 32) {
                    zmm0 = Avx512<T>::From(White, i);
                    Avx512<T>::Store(zmm0, accumulator.White, i);
                }

                for (size_t i = 0; i < AccumulatorSize; i += 32) {
                    zmm0 = Avx512<T>::From(Black, i);
                    Avx512<T>::Store(zmm0, accumulator.Black, i);
                }
#elifdef __AVX2__ // Limit this to AVX instead.
                Vec256I ymm0;

                for (size_t i = 0; i < AccumulatorSize; i += 16) {
                    ymm0 = Avx<T>::From(White, i);
                    Avx<T>::Store(ymm0, accumulator.White, i);
                }

                for (size_t i = 0; i < AccumulatorSize; i += 16) {
                    ymm0 = Avx<T>::From(Black, i);
                    Avx<T>::Store(ymm0, accumulator.Black, i);
                }
#else
                std::copy(std::begin(White), std::end(White), std::begin(accumulator.White));
                std::copy(std::begin(Black), std::end(Black), std::begin(accumulator.Black));
#endif
            }

            inline void LoadBias(std::array<T, AccumulatorSize>& bias)
            {
                std::copy(std::begin(bias), std::end(bias), std::begin(White));
                std::copy(std::begin(bias), std::end(bias), std::begin(Black));
            }

            inline void Zero()
            {
                std::fill(std::begin(White), std::end(White), 0);
                std::fill(std::begin(Black), std::end(Black), 0);
            }

    };

} // MantaRay

#endif //MANTARAY_PERSPECTIVEACCUMULATOR_H
