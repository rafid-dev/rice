//
// Copyright (c) 2023 MantaRay authors. See the list of authors for more details.
// Licensed under MIT.
//

#ifndef MANTARAY_CLIPPEDRELU_H
#define MANTARAY_CLIPPEDRELU_H

#include <utility>
#include "../Backend/Avx.h"
#include "../Backend/Avx2.h"

namespace MantaRay
{

    template<typename T, T Minimum, T Maximum>
    class ClippedReLU
    {

        public:
#ifdef __AVX512BW__
            static inline Vec512I Activate(const Vec512I& arg)
            {
                const Vec512I min = Avx512<T>::From(Minimum);
                const Vec512I max = Avx512<T>::From(Maximum);

                return Avx512<T>::Max(min, Avx512<T>::Min(max, arg));
            }
#elifdef __AVX2__
            static inline Vec256I Activate(const Vec256I& arg)
            {
                const Vec256I min = Avx<T>::From(Minimum);
                const Vec256I max = Avx<T>::From(Maximum);

                return Avx2<T>::Max(min, Avx2<T>::Min(max, arg));
            }
#else
            static inline T Activate(const T arg)
            {
                return std::max(Minimum, std::min(Maximum, arg));
            }
#endif

    };

} // MantaRay

#endif //MANTARAY_CLIPPEDRELU_H
