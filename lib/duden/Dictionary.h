#pragma once

#include "Duden.h"
#include "InfFile.h"
#include "Archive.h"
#include "LdFile.h"
#include "IFileSystem.h"
#include <string_view>
#include <string>
#include <vector>
#include <memory>

namespace duden {

class FileSystem : public IFileSystem {
    std::filesystem::path _root;
    CaseInsensitiveSet _files;

public:
    FileSystem(std::filesystem::path root);
    std::unique_ptr<common::IRandomAccessStream> open(std::filesystem::path path) override;
    const CaseInsensitiveSet& files() override;
};

class Dictionary {
    IFileSystem* _filesystem;
    InfFile _inf;
    HicFile _hic;
    LdFile _ld;
    std::unique_ptr<Archive> _articles;
    std::unique_ptr<common::IRandomAccessStream> _articlesBof;
    std::vector<HicLeaf> _leafs;

    void collectLeafs();

public:
    Dictionary(IFileSystem* filesystem, std::filesystem::path infPath, int index);
    std::string annotation() const;
    std::vector<char> icon() const;
    unsigned articleCount() const;
    unsigned articleArchiveDecodedSize() const;
    const std::vector<HicLeaf>& entries() const;
    std::vector<char> readEncoded(uint32_t plainOffset, uint32_t size);
    std::string article(uint32_t plainOffset, uint32_t size);
    const LdFile& ld() const;
    const InfFile& inf() const;
    const HicFile& hic() const;
};

} // namespace duden
