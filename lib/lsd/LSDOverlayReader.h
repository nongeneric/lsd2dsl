#pragma once

#include "lsd.h"
#include <string>
#include <stdint.h>

namespace dictlsd {

class DictionaryReader;
class LSDOverlayReader {
    DictionaryReader* _reader;
    unsigned _entriesCount;
    IBitStream* _bstr;
public:
    LSDOverlayReader(IBitStream* bstr,
                     DictionaryReader* dictionaryReader);
    std::vector<OverlayHeading> readHeadings();
    std::vector<uint8_t> readEntry(OverlayHeading const& heading);
};

}
