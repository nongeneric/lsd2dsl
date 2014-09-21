#include "DictionaryReader.h"
#include "BitStream.h"
#include "UserDictionaryDecoder.h"
#include "SystemDictionaryDecoder.h"
#include "AbbreviationDictionaryDecoder.h"
#include "tools.h"

#include <string.h>
#include <assert.h>
#include <stdexcept>
#include <boost/format.hpp>

namespace dictlsd {

DictionaryReader::DictionaryReader(IBitStream *bstr)
    : _bstr(bstr)
{
    _bstr->readSome(&_header, sizeof(LSDHeader));
    if (strcmp("LingVo", _header.magic) != 0)
        throw std::runtime_error("incorrect lsd file");
    if (_header.version == 0x142001 || _header.version == 0x152001) {
        _decoder.reset(new UserDictionaryDecoder());
    } else if (_header.version == 0x141004) {
        _decoder.reset(new SystemDictionaryDecoder(false));
    } else if (_header.version == 0x145001 || _header.version == 0x155001) {
        _decoder.reset(new AbbreviationDictionaryDecoder());
    } else if (_header.version == 0x151005) {
        _decoder.reset(new SystemDictionaryDecoder(true));
    } else {
        throw std::runtime_error(str(boost::format("not supported version %x") % _header.version));
    }

    _name = readUnicodeString(bstr, bstr->read(8), false);
    auto firstHeading = readUnicodeString(bstr, bstr->read(8), false); (void)firstHeading;
    auto lastHeading = readUnicodeString(bstr, bstr->read(8), false); (void)lastHeading;
    auto capitals = readUnicodeString(bstr, reverse32(bstr->read(32)), false); (void)capitals;
    unsigned iconLen = reverse16(bstr->read(16));
    _icon.resize(iconLen);
    bstr->readSome(&_icon[0], iconLen);
    auto checksum = bstr->read(32); (void)checksum;
    _pagesEnd = reverse32(bstr->read(32));
    _overlayData = reverse32(bstr->read(32));

    bstr->seek(_header.dictionaryEncoderOffset);
    _decoder->Read(bstr);
}

std::u16string DictionaryReader::name() {
    return _name;
}

std::u16string DictionaryReader::prefix() {
    return _decoder->Prefix();
}

std::u16string DictionaryReader::annotation() {
    _bstr->seek(_header.annotationOffset);
    std::u16string anno;
    bool decoded = _decoder->DecodeArticle(_bstr, anno);
    assert(decoded);
    return anno;
}

unsigned DictionaryReader::pagesCount() const {
    return (_pagesEnd - _header.pagesOffset) / 512;
}

unsigned DictionaryReader::overlayHeadingsOffset() const {
    return _pagesEnd;
}

std::vector<char> DictionaryReader::icon() const {
    return _icon;
}

std::u16string DictionaryReader::decodeArticle(IBitStream &bstr, unsigned reference) {
    bstr.seek(header()->articlesOffset + reference);
    std::u16string body;
    bool res = decoder()->DecodeArticle(&bstr, body);
    assert(res);
    return body;
}

IDictionaryDecoder *DictionaryReader::decoder() {
    return _decoder.get();
}

LSDHeader *DictionaryReader::header() {
    return &_header;
}

unsigned DictionaryReader::overlayDataOffset() const {
    return _overlayData;
}

}
