#pragma once

#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
#include <immintrin.h>
#define USE_SIMD
#endif

#if defined(__AVX512F__)
#define BIT_ALIGNMENT  (512)
#elif defined(__AVX2__) || defined(__AVX__)
#define BIT_ALIGNMENT  (256)
#endif

#define STRIDE_16_BIT (BIT_ALIGNMENT / 16)
#define ALIGNMENT     (BIT_ALIGNMENT / 8)
#define REG_COUNT (16)

#if defined(__AVX512F__)
using register_type16 = __m512i;
using register_type32 = __m512i;
#define register_madd_epi16 _mm512_madd_epi16
#define register_add_epi32  _mm512_add_epi32
#define register_sub_epi32  _mm512_sub_epi32
#define register_add_epi16  _mm512_add_epi16
#define register_sub_epi16  _mm512_sub_epi16
#define register_max_epi16  _mm512_max_epi16
#define register_load   _mm512_load_si512
#define register_store  _mm512_store_si512
#elif defined(__AVX2__) || defined(__AVX__)
using register_type16 = __m256i;
using register_type32 = __m256i;
#define register_madd_epi16 _mm256_madd_epi16
#define register_load   _mm256_load_si256
#define register_store  _mm256_store_si256
#define register_add_epi32  _mm256_add_epi32
#define register_sub_epi32  _mm256_sub_epi32
#define register_add_epi16  _mm256_add_epi16
#define register_sub_epi16  _mm256_sub_epi16
#define register_max_epi16  _mm256_max_epi16
#define avx_zero _mm256_setzero_si256
#endif


#if defined(USE_SIMD)
inline int32_t sumRegisterEpi32(register_type32& reg) {
#if defined(__AVX512F__)
    const __m256i reduced_8 =
        _mm256_add_epi32(_mm512_castsi512_si256(reg), _mm512_extracti32x8_epi32(reg, 1));
#elif defined(__AVX2__) || defined(__AVX__)
    const __m256i reduced_8 = reg;
#endif

    const __m128i reduced_4 =
        _mm_add_epi32(_mm256_castsi256_si128(reduced_8), _mm256_extractf128_si256(reduced_8, 1));

    __m128i vsum = _mm_add_epi32(reduced_4, _mm_srli_si128(reduced_4, 8));
    vsum         = _mm_add_epi32(vsum, _mm_srli_si128(vsum, 4));
    int32_t sums = _mm_cvtsi128_si32(vsum);
    return sums;
}
#endif