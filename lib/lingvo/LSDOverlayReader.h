#pragma once

#include "lsd.h"
#include <string>
#include <stdint.h>

namespace lingvo {

class DictionaryReader;
class LSDOverlayReader {
    DictionaryReader* _reader;
    unsigned _entriesCount;
    common::IBitStream* _bstr;
public:
    LSDOverlayReader(common::IBitStream* bstr,
                     DictionaryReader* dictionaryReader);
    std::vector<OverlayHeading> readHeadings();
    std::vector<uint8_t> readEntry(OverlayHeading const& heading);
};

}
