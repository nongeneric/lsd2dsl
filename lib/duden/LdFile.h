#pragma once

#include "common/BitStream.h"
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
    std::string baseFileName;
    std::string name;
    std::string sourceLanguage;
    int sourceLanguageCode = -1;
    int targetLanguageCode = -1;
    std::vector<ReferenceInfo> references;
    std::vector<ReferenceRange> ranges;
};

LdFile parseLdFile(common::IRandomAccessStream* stream);
void updateLanguageCodes(std::vector<LdFile*> lds);

} // namespace duden
