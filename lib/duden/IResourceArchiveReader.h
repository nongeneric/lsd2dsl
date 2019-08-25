#pragma once

#include <stdint.h>
#include <vector>

namespace duden {

class IResourceArchiveReader {
public:
    ~IResourceArchiveReader() = default;
    virtual void read(uint32_t plainOffset, uint32_t size, std::vector<char>& output) = 0;
    virtual unsigned decodedSize() const = 0;
};

} // namespace duden
