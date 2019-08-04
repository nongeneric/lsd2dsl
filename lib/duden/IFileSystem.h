#pragma once

#include "lib/lsd/BitStream.h"
#include "lib/common/filesystem.h"
#include <optional>
#include <set>

namespace duden {

struct CaseInsensitiveLess {
    bool operator()(const fs::path& left, const fs::path& right) const;
};

using CaseInsensitiveSet = std::set<fs::path, CaseInsensitiveLess>;

struct IFileSystem {
    virtual std::unique_ptr<dictlsd::IRandomAccessStream> open(fs::path path) = 0;
    virtual const CaseInsensitiveSet& files() = 0;
    ~IFileSystem() = default;
};

fs::path findExtension(IFileSystem& fileSystem, fs::path name);

} // namespace duden
