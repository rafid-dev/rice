//
// Copyright (c) 2023 MantaRay authors. See the list of authors for more details.
// Licensed under MIT.
//

#ifndef MANTARAY_BINARYMEMORYSTREAM_H
#define MANTARAY_BINARYMEMORYSTREAM_H

#include <streambuf>
#include <istream>
#include <array>
#include <cstdint>

#include "DataStream.h"

namespace MantaRay
{

    using streambuf = std::basic_streambuf<unsigned char, std::char_traits<unsigned char>>;

    struct BinaryMemoryBuffer : streambuf
    {

        public:
            BinaryMemoryBuffer(const unsigned char* src, const size_t size) {
                auto *p (const_cast<unsigned char*>(src));
                this->setg(p, p, p + size);
            }

    };

    using istream = std::basic_istream<unsigned char, std::char_traits<unsigned char>>;

    struct BinaryMemoryStream : virtual MantaRay::BinaryMemoryBuffer, istream
    {

        public:
            __attribute__((unused)) BinaryMemoryStream(const unsigned char* src, const size_t size) :
            BinaryMemoryBuffer(src, size), istream(static_cast<streambuf*>(this)) {}

            template<typename T, size_t Size>
            void ReadArray(std::array<T, Size> &array)
            {
                this->read((unsigned char*)(&array), sizeof array);
            }

    };

} // MantaRay

#endif //MANTARAY_BINARYMEMORYSTREAM_H
