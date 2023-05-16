//
// Copyright (c) 2023 MantaRay authors. See the list of authors for more details.
// Licensed under MIT.
//

#ifndef MANTARAY_REGISTERDEFINITION_H
#define MANTARAY_REGISTERDEFINITION_H

#include "immintrin.h"

#ifdef __AVX512F__
using Vec512I = __m512i;
using Vec256I = __m256i;
using Vec128I = __m128i;
#elif __AVX__
using Vec256I = __m256i;
using Vec128I = __m128i;
#elif __SSE__
using Vec128I = __m128i;
#endif

#endif //MANTARAY_REGISTERDEFINITION_H
