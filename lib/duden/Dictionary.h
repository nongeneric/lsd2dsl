#pragma once

#include "Duden.h"
#include "InfFile.h"
#include "Archive.h"
#include "LdFile.h"
#include "lib/common/filesystem.h"
#include "IFileSystem.h"
#include <string_view>
#include <string>
#include <vector>
#include <memory>

namespace duden {

class FileSystem : public IFileSystem {
    fs::path _root;
    CaseInsensitiveSet _files;

public:
    FileSystem(fs::path root);
    std::unique_ptr<dictlsd::IRandomAccessStream> open(fs::path path) override;
    const CaseInsensitiveSet& files() override;
};

class Dictionary {
    IFileSystem* _filesystem;
    InfFile _inf;
    HicFile _hic;
    LdFile _ld;
    std::unique_ptr<Archive> _articles;
    std::unique_ptr<dictlsd::IRandomAccessStream> _articlesBof;
    std::vector<HicLeaf> _leafs;

    void collectLeafs();

public:
    Dictionary(IFileSystem* filesystem, fs::path infPath, int index);
    std::string annotation() const;
    std::vector<char> icon() const;
    unsigned articleCount() const;
    unsigned articleArchiveDecodedSize() const;
    const std::vector<HicLeaf>& entries() const;
    std::string article(uint32_t plainOffset, uint32_t size);
    const LdFile& ld() const;
    const InfFile& inf() const;
};

} // namespace duden
