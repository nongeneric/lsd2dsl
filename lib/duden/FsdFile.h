#pragma once

#include "common/BitStream.h"
#include "IResourceArchiveReader.h"
#include <vector>
#include <memory>

namespace duden {

class FsdFile : public IResourceArchiveReader {
    std::shared_ptr<common::IRandomAccessStream> _stream;

public:
    FsdFile(std::shared_ptr<common::IRandomAccessStream> stream);
    void read(uint32_t plainOffset, uint32_t size, std::vector<char>& output) override;
    unsigned decodedSize() const override;
};

} // namespace duden
