//
// Copyright (c) 2023 MantaRay authors. See the list of authors for more details.
// Licensed under MIT.
//

#ifndef MANTARAY_MARLINFLOWSTREAM_H
#define MANTARAY_MARLINFLOWSTREAM_H

#include <array>
#include <cstdint>
#include "DataStream.h"
#include "../External/json.hpp"

using JSON = nlohmann::json;

namespace MantaRay
{

    class MarlinflowStream : DataStream<std::ios::in>
    {

        private:
            JSON data;

        public:
            __attribute__((unused)) explicit MarlinflowStream(const std::string& path) : DataStream(path)
            {
                data = JSON::parse(this->Stream);
            }

            template<typename T, size_t Size>
            void Read2DArray(const std::string& key, std::array<T, Size>& array, const size_t stride, const size_t K,
                             const bool permute)
            {
                JSON &obj = data[key];

                size_t i = 0;
                for (auto &[k, v] : obj.items()) {
                    size_t j = 0;

                    for (auto &[k2, v2] : v.items()) {
                        size_t idx = permute ? j * stride + i : i * stride + j;

                        auto d     = static_cast<double>(v2);
                        array[idx] = static_cast<T>(d * K);
                        j++;
                    }

                    i++;
                }
            }

            template<typename T, size_t Size>
            void ReadArray(const std::string& key, std::array<T, Size>& array, const size_t K)
            {
                JSON &obj = data[key];

                size_t i = 0;
                for (auto &[k, v] : obj.items()) {
                    auto d   = static_cast<double>(v);
                    array[i] = static_cast<T>(d * K);
                    i++;
                }
            }

    };

} // MantaRay

#endif //MANTARAY_MARLINFLOWSTREAM_H
