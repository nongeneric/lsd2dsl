#pragma once

#include "lib/lsd/BitStream.h"
#include "IFileSystem.h"

namespace duden {

struct PrimaryArchive {
    std::string bof;
    std::string idx;
    std::string hic;
};

struct ResourceArchive {
    std::string bof;
    std::string idx;
    std::string fsi;
};

struct InfFile {
    int version;
    bool supported;
    std::string ld;
    PrimaryArchive primary;
    std::vector<ResourceArchive> resources;
};

std::vector<InfFile> parseInfFile(dictlsd::IRandomAccessStream* stream, IFileSystem* filesystem);

} // namespace duden
