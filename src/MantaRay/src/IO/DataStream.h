//
// Copyright (c) 2023 MantaRay authors. See the list of authors for more details.
// Licensed under MIT.
//

#ifndef MANTARAY_DATASTREAM_H
#define MANTARAY_DATASTREAM_H

#include <iostream>
#include <fstream>
#include <ios>

#define FileStream std::fstream

namespace MantaRay
{

    template<std::ios_base::openmode O>
    class DataStream
    {

        protected:
            FileStream Stream;

        public:
            explicit DataStream(const std::string& path)
            {
                Stream.open(path, O);
            }
    };

} // MantaRay

#endif //MANTARAY_DATASTREAM_H
