//
// Copyright (c) 2023 MantaRay authors. See the list of authors for more details.
// Licensed under MIT.
//

#ifndef MANTARAY_AVX2_H
#define MANTARAY_AVX2_H

#include "Avx.h"

namespace MantaRay
{

    template<typename T>
    class Avx2
    {

        static_assert(std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> || std::is_same_v<T, int32_t>,
                      "Unsupported type provided.");

        public:
            static inline Vec256I Min(const Vec256I& ymm0, const Vec256I& ymm1)
            {
                if (std::is_same_v<T, int8_t> ) return _mm256_min_epi8 (ymm0, ymm1);

                if (std::is_same_v<T, int16_t>) return _mm256_min_epi16(ymm0, ymm1);

                if (std::is_same_v<T, int32_t>) return _mm256_min_epi32(ymm0, ymm1);
            }

            static inline Vec256I Max(const Vec256I& ymm0, const Vec256I& ymm1)
            {
                if (std::is_same_v<T, int8_t> ) return _mm256_max_epi8 (ymm0, ymm1);

                if (std::is_same_v<T, int16_t>) return _mm256_max_epi16(ymm0, ymm1);

                if (std::is_same_v<T, int32_t>) return _mm256_max_epi32(ymm0, ymm1);
            }

            static inline Vec256I Add(const Vec256I& ymm0, const Vec256I& ymm1)
            {
                if (std::is_same_v<T, int8_t> ) return _mm256_add_epi8 (ymm0, ymm1);

                if (std::is_same_v<T, int16_t>) return _mm256_add_epi16(ymm0, ymm1);

                if (std::is_same_v<T, int32_t>) return _mm256_add_epi32(ymm0, ymm1);
            }

            static inline Vec256I Subtract(const Vec256I& ymm0, const Vec256I& ymm1)
            {
                if (std::is_same_v<T, int8_t> ) return _mm256_sub_epi8 (ymm0, ymm1);

                if (std::is_same_v<T, int16_t>) return _mm256_sub_epi16(ymm0, ymm1);

                if (std::is_same_v<T, int32_t>) return _mm256_sub_epi32(ymm0, ymm1);
            }

            static inline Vec256I MultiplyAndAddAdjacent(const Vec256I& ymm0, const Vec256I& ymm1)
            {
                static_assert(std::is_same_v<T, int16_t>, "Unsupported type provided.");

                return _mm256_madd_epi16(ymm0, ymm1);
            }

            static inline T Sum(const Vec256I& ymm0)
            {
                static_assert(std::is_same_v<T, int32_t>, "Unsupported type provided.");

                Vec128I xmm0;
                Vec128I xmm1;

                xmm0 = _mm256_castsi256_si128(ymm0);
                xmm1 = _mm256_extracti128_si256(ymm0, 1);
                xmm0 = _mm_add_epi32(xmm0, xmm1);
                xmm1 = _mm_unpackhi_epi64(xmm0, xmm0);
                xmm0 = _mm_add_epi32(xmm0, xmm1);
                xmm1 = _mm_shuffle_epi32(xmm0, _MM_SHUFFLE(2, 3, 0, 1));
                xmm0 = _mm_add_epi32(xmm0, xmm1);
                return _mm_cvtsi128_si32(xmm0);
            }

    };

} // MantaRay

#endif //MANTARAY_AVX2_H
