#include "SystemDictionaryDecoder.h"
#include "common/BitStream.h"
#include "tools.h"

namespace lingvo {

SystemDictionaryDecoder::SystemDictionaryDecoder(bool xoring)
    : _xoring(xoring) { }

bool SystemDictionaryDecoder::DecodeArticle(
        common::IBitStream *bstr,
        std::u16string &res,
        std::u16string const& prefix,
        bool xoring,
        LenTable& ltArticles,
        std::vector<char32_t>& articleSymbols)
{
    common::XoringStreamAdapter adapter(bstr);
    if (xoring) {
        bstr = &adapter;
    }
    unsigned maxlen = bstr->read(16);
    if (maxlen == 0xFFFF) {
        maxlen = bstr->read(32);
    }
    res.clear();
    unsigned symIdx;
    while ((unsigned)res.length() < maxlen) {
        ltArticles.Decode(*bstr, symIdx);
        unsigned sym = articleSymbols.at(symIdx);
        if (sym - 0x80 >= 0x10000) {
            if (sym <= 0x3F) {
                unsigned startIdx = bstr->read(BitLength(prefix.length()));
                unsigned len = sym + 3;
                res += prefix.substr(startIdx, len);
            } else {
                unsigned startIdx = bstr->read(BitLength(maxlen));
                unsigned len = sym - 0x3d;
                res += res.substr(startIdx, len);
            }
        } else {
            res += (char16_t)(sym - 0x80);
        }
    }
    return true;
}

void SystemDictionaryDecoder::Read(common::IBitStream *bstr) {
    common::XoringStreamAdapter adapter(bstr);
    if (_xoring) {
        bstr = &adapter;
    }
    int len = bstr->read(32);
    _prefix = readUnicodeString(bstr, len, true);
    _articleSymbols = readSymbols(bstr);
    _headingSymbols = readSymbols(bstr);
    _ltArticles.Read(*bstr);
    _ltHeadings.Read(*bstr);

    _ltPostfixLengths.Read(*bstr);
    bstr->read(32);
    _ltPrefixLengths.Read(*bstr);

    _huffman1Number = bstr->read(32);
    _huffman2Number = bstr->read(32);
}

void SystemDictionaryDecoder::DecodeHeading(common::IBitStream *bstr, unsigned len, std::u16string &res) {
    res.clear();
    unsigned symIdx;
    for (size_t i = 0; i < len; i++) {
        _ltHeadings.Decode(*bstr, symIdx);
        unsigned sym = _headingSymbols.at(symIdx);
        assert(sym <= 0xffff);
        res += (char16_t)sym;
    }
}

bool SystemDictionaryDecoder::DecodeArticle(common::IBitStream *bstr, std::u16string &res) {
    return DecodeArticle(bstr, res, _prefix, _xoring, _ltArticles, _articleSymbols);
}

bool SystemDictionaryDecoder::DecodePrefixLen(common::IBitStream &bstr, unsigned &len) {
    return _ltPrefixLengths.Decode(bstr, len);
}

bool SystemDictionaryDecoder::DecodePostfixLen(common::IBitStream &bstr, unsigned &len) {
    return _ltPostfixLengths.Decode(bstr, len);
}

bool SystemDictionaryDecoder::ReadReference1(common::IBitStream &bstr, unsigned &reference) {
    return readReference(bstr, reference, _huffman1Number);
}

bool SystemDictionaryDecoder::ReadReference2(common::IBitStream &bstr, unsigned &reference) {
    return readReference(bstr, reference, _huffman2Number);
}

std::u16string SystemDictionaryDecoder::Prefix() {
    return _prefix;
}

}
