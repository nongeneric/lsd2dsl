#pragma once

#include "common/BitStream.h"
#include <optional>
#include <memory>
#include <set>
#include <map>

namespace duden {

struct CaseInsensitiveLess {
    bool operator()(const std::filesystem::path& left, const std::filesystem::path& right) const;
};

using CaseInsensitiveSet = std::set<std::filesystem::path, CaseInsensitiveLess>;

template <class T>
using CaseInsensitiveMap = std::map<std::filesystem::path, T, CaseInsensitiveLess>;

struct IFileSystem {
    virtual std::unique_ptr<common::IRandomAccessStream> open(std::filesystem::path path) = 0;
    virtual const CaseInsensitiveSet& files() = 0;
    ~IFileSystem() = default;
};

} // namespace duden
