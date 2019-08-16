#pragma once

#include "lib/lsd/BitStream.h"
#include <vector>
#include <string>
#include "stdint.h"

namespace duden {

struct ReferenceRange {
    std::string fileName;
    uint32_t first;
    uint32_t last;
};

struct ReferenceInfo {
    std::string type;
    std::string name;
    std::string code;
};

struct LdFile {
    int sourceLanguage;
    int targetLanguage;
    std::vector<ReferenceInfo> references;
    std::vector<ReferenceRange> ranges;
};

LdFile parseLdFile(dictlsd::IRandomAccessStream* stream);

} // namespace duden
