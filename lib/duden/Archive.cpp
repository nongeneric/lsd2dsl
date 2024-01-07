#include "Archive.h"

#include "assert.h"

namespace duden {

inline constexpr unsigned g_DecodedBofBlockSize = 0x2000;

bool Archive::readBlock(uint32_t index) {
    if (index == _lastBlock)
        return true;
    auto offset = _index.at(index);
    auto size = _index.at(index + 1) - offset;
    if (!size)
        return false;
    _bofBuf.resize(size);
    _bof->seek(offset);
    _bof->readSome(_bofBuf.data(), _bofBuf.size());
    decodeBofBlock(_bofBuf.data(), _bofBuf.size(), _decodedBofBlock);
    if (_decodedBofBlock.size() > g_DecodedBofBlockSize)
        throw std::runtime_error("bof block is too large");
    _lastBlock = index;
    return true;
}

Archive::Archive(common::IRandomAccessStream* index,
                 std::shared_ptr<common::IRandomAccessStream> bof)
    : _bof(bof) {
    _index = parseIndex(index);
    _decodedSize = _index.back();
    _index.pop_back();
}

void Archive::read(uint32_t plainOffset,
                   uint32_t size,
                   std::vector<char>& output) {
    if (plainOffset >= _decodedSize)
        throw std::runtime_error("reading past the end of archive");
    output.resize(0);
    auto block = plainOffset / g_DecodedBofBlockSize;
    auto offset = plainOffset % g_DecodedBofBlockSize;
    while (output.size() != size && readBlock(block)) {
        auto b = begin(_decodedBofBlock) + offset;
        auto e = end(_decodedBofBlock);
        auto toCopy = std::min<size_t>(std::distance(b, e), size - output.size());
        std::copy(b, b + toCopy, std::back_inserter(output));
        block++;
        offset = 0;
    }
}

unsigned Archive::decodedSize() const {
    return _decodedSize;
}

} // namespace duden
