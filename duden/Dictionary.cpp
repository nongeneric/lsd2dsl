#include "Dictionary.h"
#include <boost/algorithm/string.hpp>

using namespace dictlsd;

namespace duden {

Dictionary::Dictionary(IFileSystem* filesystem, const InfFile& inf)
    : _filesystem(filesystem), _inf(inf) {
    auto hicStream = _filesystem->open(_inf.primary.hic);
    _hic = parseHicFile(hicStream.get());
    auto ldPath = findExtension(*filesystem, ".ld");
    auto ldStream = _filesystem->open(ldPath.filename());
    _ld = parseLdFile(ldStream.get());
}

unsigned Dictionary::articleCount() const {
    return _hic.entries.size();
}

const std::vector<HicEntry>& Dictionary::entries() const {
    return _hic.entries;
}

std::string Dictionary::article(uint32_t plainOffset, uint32_t size) {
    if (!_articles) {
        auto idxStream = _filesystem->open(_inf.primary.idx);
        _articlesBof = _filesystem->open(_inf.primary.bof);
        _articles = std::make_unique<Archive>(idxStream.get(), _articlesBof.get());
    }
    std::vector<char> buf;
    _articles->read(plainOffset, size, buf);
    return dudenToUtf8(std::string(begin(buf), end(buf)));
}

const LdFile& Dictionary::ld() const {
    return _ld;
}

FileSystem::FileSystem(fs::path root) : _root(root) {}

std::unique_ptr<dictlsd::IRandomAccessStream> FileSystem::open(fs::path path) {
    auto absolute = _root / path;
    return std::unique_ptr<IRandomAccessStream>(new FileStream(absolute.string()));
}

std::vector<fs::path> FileSystem::files() {
    std::vector<fs::path> vec;
    for (auto& p : fs::directory_iterator(_root)) {
        vec.push_back(p.path());
    }
    return vec;
}

} // namespace duden
