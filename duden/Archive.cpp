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
    _bof->readSome(&_bofBuf[0], _bofBuf.size());
    _decodedBofBlock.resize(g_DecodedBofBlockSize);
    decodeBofBlock(&_bofBuf[0], _bofBuf.size(), _decodedBofBlock);
    if (_decodedBofBlock.size() > g_DecodedBofBlockSize)
        throw std::runtime_error("bof block is too large");
    _lastBlock = index;
    return true;
}

Archive::Archive(dictlsd::IRandomAccessStream* index,
                 dictlsd::IRandomAccessStream* bof)
    : _bof(bof) {
    _index = parseIndex(index);
}

void Archive::read(uint32_t plainOffset,
                   uint32_t size,
                   std::vector<char>& output) {
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

} // namespace duden
