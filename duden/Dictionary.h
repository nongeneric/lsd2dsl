#pragma once

#include "Duden.h"
#include "InfFile.h"
#include "Archive.h"
#include "LdFile.h"
#include "dictlsd/filesystem.h"
#include "IFileSystem.h"
#include <string_view>
#include <string>
#include <vector>
#include <memory>

namespace duden {

class FileSystem : public IFileSystem {
    fs::path _root;

public:
    FileSystem(fs::path root);
    std::unique_ptr<dictlsd::IRandomAccessStream> open(fs::path path) override;
    std::vector<fs::path> files() override;
};

class Dictionary {
    IFileSystem* _filesystem;
    InfFile _inf;
    HicFile _hic;
    LdFile _ld;
    std::unique_ptr<Archive> _articles;
    std::unique_ptr<dictlsd::IRandomAccessStream> _articlesBof;

public:
    Dictionary(IFileSystem* filesystem, InfFile const& inf);
    std::string annotation() const;
    std::vector<char> icon() const;
    unsigned articleCount() const;
    const std::vector<HicEntry>& entries() const;
    std::string article(uint32_t plainOffset, uint32_t size);
    const LdFile& ld() const;
};

} // namespace duden
