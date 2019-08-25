#pragma once

#include "lib/lsd/BitStream.h"
#include "IResourceArchiveReader.h"
#include <vector>
#include <memory>

namespace duden {

class FsdFile : public IResourceArchiveReader {
    std::shared_ptr<dictlsd::IRandomAccessStream> _stream;

public:
    FsdFile(std::shared_ptr<dictlsd::IRandomAccessStream> stream);
    void read(uint32_t plainOffset, uint32_t size, std::vector<char>& output) override;
    unsigned decodedSize() const override;
};

} // namespace duden
