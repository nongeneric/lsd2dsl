#pragma once

#include "BitStream.h"
#include <string>
#include <vector>
#include <functional>

namespace dictlsd {

struct LSAEntry {
    std::u16string name;
    unsigned sampleOffset; // easily overflowed for very big archives
    unsigned sampleSize;
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
              int initialProgress,
              std::function<void(int)> log);
    unsigned entriesCount() const;
};

void decodeLSA(std::string lsaPath, std::string outputPath, std::function<void(int)> log);

}
