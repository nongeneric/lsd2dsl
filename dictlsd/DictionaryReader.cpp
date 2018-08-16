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

void DictionaryReader::loadDecoder() const {
    if (!_isSupported)
        throw std::runtime_error("unsuported dictionary version");
    if (!_decoderLoaded) {
        auto pos = _bstr->tell();
        _bstr->seek(_header.dictionaryEncoderOffset);
        _decoder->Read(_bstr);
        _decoderLoaded = true;
        _bstr->seek(pos);
    }
}

DictionaryReader::DictionaryReader(IBitStream *bstr)
    : _bstr(bstr), _isSupported(true), _decoderLoaded(false)
{
    _bstr->readSome(&_header, sizeof(LSDHeader));
    if (strcmp("LingVo", _header.magic) != 0)
        throw NotLSDException();
    if (_header.version == 0x132001 || _header.version == 0x142001 || _header.version == 0x152001) {
        _decoder.reset(new UserDictionaryDecoder(false));
    } else if (_header.version == 0x141004) {
        _decoder.reset(new SystemDictionaryDecoder(false));
    } else if (_header.version == 0x131001) {
        _decoder.reset(new UserDictionaryDecoder(true));
    } else if (_header.version == 0x145001 || _header.version == 0x155001) {
        _decoder.reset(new AbbreviationDictionaryDecoder());
    } else if (_header.version == 0x151005) {
        _decoder.reset(new SystemDictionaryDecoder(true));
    } else {
        _isSupported = false;
        return;
    }

    uint8_t nameLen;
    bstr->readSome(&nameLen, 1);
    _name = readUnicodeString(bstr, nameLen, false);
    auto firstHeading = readUnicodeString(bstr, bstr->read(8), false); (void)firstHeading;
    auto lastHeading = readUnicodeString(bstr, bstr->read(8), false); (void)lastHeading;    
    auto capitals = readUnicodeString(bstr, reverse32(bstr->read(32)), false); (void)capitals;
    uint16_t iconLen;
    bstr->readSome(&iconLen, 2);
    _icon.resize(iconLen);
    bstr->readSome(&_icon[0], iconLen);
    if (_header.version > 0x140000) {
        bstr->seek(bstr->tell() + 4); // checksum
    }
    bstr->readSome(&_pagesEnd, 4);
    bstr->readSome(&_overlayData, 4);
    if (_header.version < 0x140000) {
        _overlayData = 0; // headings use absolute offsets
    }
}

bool DictionaryReader::supported() const {
    return _isSupported;
}

std::u16string DictionaryReader::name() const {
    return _name;
}

std::u16string DictionaryReader::prefix() const {
    loadDecoder();
    return _decoder->Prefix();
}

std::u16string DictionaryReader::annotation() const {
    loadDecoder();
    _bstr->seek(_header.annotationOffset);
    std::u16string anno;
    bool decoded = _decoder->DecodeArticle(_bstr, anno);
    if (!decoded)
        throw std::runtime_error("can't decode annotation");
    return anno;
}

unsigned DictionaryReader::pagesCount() const {
    return (_pagesEnd - _header.pagesOffset) / 512;
}

unsigned DictionaryReader::overlayHeadingsOffset() const {
    return _pagesEnd;
}

std::vector<unsigned char> const& DictionaryReader::icon() const {
    return _icon;
}

std::u16string DictionaryReader::decodeArticle(IBitStream &bstr, unsigned reference) {
    loadDecoder();
    bstr.seek(header().articlesOffset + reference);
    std::u16string body;
    bool res = decoder()->DecodeArticle(&bstr, body);
    if (!res)
        throw std::runtime_error("can't decode article");
    return body;
}

IDictionaryDecoder *DictionaryReader::decoder() {
    loadDecoder();
    return _decoder.get();
}

LSDHeader const& DictionaryReader::header() const {
    return _header;
}

unsigned DictionaryReader::overlayDataOffset() const {
    return _overlayData;
}

const char *NotLSDException::what() const noexcept {
    return "Not an LSD file.";
}

}
