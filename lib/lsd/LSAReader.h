#pragma once

#include "BitStream.h"
#include "lib/common/Log.h"
#include <string>
#include <vector>
#include <functional>

namespace dictlsd {

struct LSAEntry {
    std::u16string name;
    uint32_t sampleOffset; // easily overflowed for very big archives
    uint32_t sampleSize;
};

class LSAReader {
    IRandomAccessStream* _bstr;
    std::vector<LSAEntry> _entries;
    unsigned _entriesCount;
    unsigned long long _totalSamples;
    unsigned _oggOffset;
public:
    LSAReader(IRandomAccessStream* bstr);
    void collectHeadings();
    void dump(std::string path,
              Log& log);
    unsigned entriesCount() const;
};

void decodeLSA(std::string lsaPath, std::string outputPath, Log& log);

}
