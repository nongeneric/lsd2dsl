#pragma once

#include "common/BitStream.h"
#include <vector>
#include <memory>

struct OggVorbis_File;

struct VorbisInfo {
    int channels;
    long rate;
};

namespace lingvo {

class OggReader {
    std::unique_ptr<OggVorbis_File> _vfile;
    int _vbitstream;
public:
    OggReader(common::IRandomAccessStream* bstr);
    // little endian signed mono
    void readSamples(unsigned count, std::vector<short>& vec);
    uint64_t totalSamples();
    VorbisInfo info();
    ~OggReader();
};

}
