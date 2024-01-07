#include "FsdFile.h"
#include <limits>

namespace duden {

FsdFile::FsdFile(std::shared_ptr<common::IRandomAccessStream> stream) : _stream(stream) {}

void FsdFile::read(uint32_t plainOffset, uint32_t size, std::vector<char>& output) {
    output.resize(size);
    std::fill(begin(output), end(output), 0);
    _stream->seek(plainOffset);
    _stream->readSome(output.data(), size);
}

unsigned FsdFile::decodedSize() const {
    return std::numeric_limits<unsigned>::max();
}

} // namespace duden
