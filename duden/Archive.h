#pragma once

#include "Duden.h"
#include <istream>
#include <stdint.h>
#include <vector>

namespace duden {

class Archive {
    std::vector<uint32_t> _index;
    dictlsd::IRandomAccessStream* _bof;
    std::vector<char> _bofBuf;
    std::vector<char> _decodedBofBlock;
    unsigned _lastBlock = -1;

    bool readBlock(uint32_t index);

public:
    Archive(dictlsd::IRandomAccessStream* index,
            dictlsd::IRandomAccessStream* bof);
    void read(uint32_t plainOffset, uint32_t size, std::vector<char>& output);
};

} // namespace duden
