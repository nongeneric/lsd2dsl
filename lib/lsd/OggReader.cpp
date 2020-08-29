#include "OggReader.h"
#include "BitStream.h"

#include <vorbis/vorbisfile.h>
#include <stdexcept>
#include <assert.h>

namespace dictlsd {

size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource) {
    auto bstr = static_cast<IRandomAccessStream*>(datasource);
    bstr->readSome(ptr, size * nmemb);
    return nmemb;
}

ov_callbacks callbacks {
    read_func,
    NULL,
    NULL,
    NULL
};

OggReader::OggReader(IRandomAccessStream *bstr)
    : _vbitstream(0)
{
    _vfile.reset(new OggVorbis_File());
    int res = ov_open_callbacks(bstr, _vfile.get(), NULL, 0, callbacks);
    if (res) {
        throw std::runtime_error("can't read ogg file");
    }
}

const unsigned BUFF_SIZE = 4096;
short buffer[BUFF_SIZE / 2];

void OggReader::readSamples(unsigned count, std::vector<short> &vec) {
    vec.clear();
    count *= 2; // samples -> bytes
    while (count) {
        unsigned toRead = std::min(count, BUFF_SIZE);
        long bytesRead = ov_read(_vfile.get(), (char*)buffer, toRead, 0, 2, 1, &_vbitstream);
        if (bytesRead == OV_HOLE ||
            bytesRead == OV_EBADLINK ||
            bytesRead == OV_EINVAL)
        {
            throw std::runtime_error("error reading samples");
        }
        if (bytesRead == 0) {
            throw std::runtime_error("unexpected eof");
        }
        count -= bytesRead;
        std::copy(buffer, buffer + bytesRead / sizeof(short), std::back_inserter(vec));
    }
}

uint64_t OggReader::totalSamples() {
    return ov_pcm_total(_vfile.get(), -1);
}

VorbisInfo OggReader::info() {
    VorbisInfo res;
    auto info = ov_info(_vfile.get(), -1);
    res.channels = info->channels;
    res.rate = info->rate;
    return res;
}

OggReader::~OggReader() { }

}
