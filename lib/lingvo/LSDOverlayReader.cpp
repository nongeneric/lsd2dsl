#include "LSDOverlayReader.h"

#include "common/BitStream.h"
#include "DictionaryReader.h"
#include "tools.h"

#include <zlib.h>
#include <stdexcept>
#include <assert.h>

namespace lingvo {

void zlibInflate(std::vector<uint8_t>& res,
                 std::vector<uint8_t> const& buf,
                 unsigned inflatedSize)
{
    res.resize(inflatedSize);

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    int ret = inflateInit(&strm);
    if (ret != Z_OK)
        throw std::runtime_error("zlib init failed");

    strm.avail_in = buf.size();
    strm.next_in = (Bytef*)buf.data();
    strm.avail_out = inflatedSize;
    strm.next_out = (Bytef*)&res[0];

    ret = inflate(&strm, Z_FINISH);
    assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
    switch (ret) {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
        inflateEnd(&strm);
        throw std::runtime_error("zlib error");
    }
    if (ret != Z_STREAM_END)
        std::runtime_error("zlib inflate failed");
    inflateEnd(&strm);
}

LSDOverlayReader::LSDOverlayReader(common::IBitStream* bstr,
                                   DictionaryReader *dictionaryReader)
    : _reader(dictionaryReader), _bstr(bstr)
{ }

std::vector<OverlayHeading> LSDOverlayReader::readHeadings() {
    auto offset = _reader->overlayDataOffset();
    if (offset == -1u)
        return {};
    _bstr->seek(_reader->overlayHeadingsOffset());
    _bstr->readSome(&_entriesCount, 4);
    std::vector<OverlayHeading> entries;
    for (unsigned i = 0; i < _entriesCount; ++i) {
        OverlayHeading entry;
        unsigned nameLen = _bstr->read(8);
        entry.name = readUnicodeString(_bstr, nameLen, false);
        _bstr->readSome(&entry.offset, 4);
        _bstr->readSome(&entry.unk2, 4);
        _bstr->readSome(&entry.inflatedSize, 4);
        _bstr->readSome(&entry.streamSize, 4);
        if (entry.inflatedSize) {
            entries.push_back(entry);
        }
    }
    return entries;
}

std::vector<uint8_t> LSDOverlayReader::readEntry(OverlayHeading const& heading) {
    _bstr->seek(heading.offset + _reader->overlayDataOffset());
    std::vector<uint8_t> slice(heading.streamSize);
    _bstr->readSome(slice.data(), heading.streamSize);
    std::vector<uint8_t> res;
    zlibInflate(res, slice, heading.inflatedSize);
    return res;
}

}
