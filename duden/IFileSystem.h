#pragma once

#include "dictlsd/BitStream.h"
#include "tools/filesystem.h"

namespace duden {

struct IFileSystem {
    virtual std::unique_ptr<dictlsd::IRandomAccessStream> open(fs::path path) = 0;
    virtual std::vector<fs::path> files() = 0;
    ~IFileSystem() = default;
};

fs::path findCaseInsensitive(IFileSystem& fileSystem, fs::path fileName);
fs::path findExtension(IFileSystem& fileSystem, fs::path name);

} // namespace duden
