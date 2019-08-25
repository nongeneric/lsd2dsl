#include "Dictionary.h"
#include <boost/algorithm/string.hpp>
#include "lib/common/overloaded.h"
#include "lib/common/bformat.h"

using namespace dictlsd;

namespace duden {

void Dictionary::collectLeafs() {
    auto collect = [&](auto& collect, auto& page) -> void {
        for (auto& entry : page->entries) {
            std::visit(overloaded{
                [&](HicLeaf& leaf) {
                   _leafs.push_back(leaf);
                },
                [&](HicNode& node) {
                   collect(collect, node.page);
                }
            }, entry);
        }
    };
    collect(collect, _hic.root);
}

Dictionary::Dictionary(IFileSystem* filesystem, fs::path infPath, int index)
    : _filesystem(filesystem) {
    auto infStream = _filesystem->open(infPath.filename());
    auto infs = parseInfFile(infStream.get(), filesystem);
    _inf = infs.at(index);

    auto ldStream = _filesystem->open(_inf.ld);
    _ld = parseLdFile(ldStream.get());

    if (infs.size() == 1) {
        updateLanguageCodes({&_ld});
    } else {
        auto secondaryLdStream = _filesystem->open(infs.at(index == 1 ? 0 : 1).ld);
        auto secondaryLd = parseLdFile(secondaryLdStream.get());
        updateLanguageCodes({&_ld, &secondaryLd});
    }

    auto hicStream = _filesystem->open(_inf.primary.hic);
    _hic = parseHicFile(hicStream.get());
    auto idxStream = _filesystem->open(_inf.primary.idx);
    _articlesBof = _filesystem->open(_inf.primary.bof);
    _articles = std::make_unique<Archive>(idxStream.get(), _articlesBof.get());
    collectLeafs();
}

unsigned Dictionary::articleCount() const {
    unsigned count = 0;
    for (auto& leaf : _leafs) {
        if (leaf.type == HicEntryType::Plain || leaf.type == HicEntryType::Variant) {
            ++count;
        }
    }
    return count;
}

unsigned Dictionary::articleArchiveDecodedSize() const {
    return _articles->decodedSize();
}

const std::vector<HicLeaf>& Dictionary::entries() const {
    return _leafs;
}

std::vector<char> Dictionary::readEncoded(uint32_t plainOffset, uint32_t size) {
    std::vector<char> buf;
    _articles->read(plainOffset, size, buf);
    return buf;
}

std::string Dictionary::article(uint32_t plainOffset, uint32_t size) {
    std::vector<char> buf;
    _articles->read(plainOffset, size, buf);
    return dudenToUtf8(std::string(begin(buf), end(buf)));
}

const LdFile& Dictionary::ld() const {
    return _ld;
}

const InfFile& Dictionary::inf() const {
    return _inf;
}

FileSystem::FileSystem(fs::path root) : _root(root) {}

std::unique_ptr<dictlsd::IRandomAccessStream> FileSystem::open(fs::path path) {
    auto absolute = _root / path;
    return std::unique_ptr<IRandomAccessStream>(new FileStream(absolute.string()));
}

const CaseInsensitiveSet& FileSystem::files() {
    if (!_files.empty())
        return _files;
    for (auto& p : fs::directory_iterator(_root)) {
        _files.insert(p.path().filename());
    }
    return _files;
}

} // namespace duden
