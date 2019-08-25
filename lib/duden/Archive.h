#pragma once

#include "Duden.h"
#include "IResourceArchiveReader.h"
#include <istream>
#include <stdint.h>
#include <vector>
#include <memory>

namespace duden {

class Archive : public IResourceArchiveReader {
    std::vector<uint32_t> _index;
    std::shared_ptr<dictlsd::IRandomAccessStream> _bof;
    std::vector<char> _bofBuf;
    std::vector<char> _decodedBofBlock;
    unsigned _lastBlock = -1;
    unsigned _decodedSize = 0;

    bool readBlock(uint32_t index);

public:
    Archive(dictlsd::IRandomAccessStream* index,
            std::shared_ptr<dictlsd::IRandomAccessStream> bof);
    void read(uint32_t plainOffset, uint32_t size, std::vector<char>& output) override;
    unsigned decodedSize() const override;
};

} // namespace duden
