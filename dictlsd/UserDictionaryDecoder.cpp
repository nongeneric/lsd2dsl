#include "UserDictionaryDecoder.h"
#include "tools.h"

#include <stdint.h>
#include <string.h>
#include <stdexcept>
#include <fstream>

namespace dictlsd {

bool UserDictionaryDecoder::DecodeArticle(
        IBitStream *bstr,
        std::u16string &res,
        std::u16string const& prefix,
        LenTable& ltArticles,
        std::vector<char32_t>& articleSymbols)
{
    unsigned len = bstr->read(16);
    if (len == 0xFFFF) {
        len = bstr->read(32);
    }
    res.clear();
    unsigned symIdx;
    std::vector<uint32_t> vec;
    while ((unsigned)res.length() < len) {
        ltArticles.Decode(*bstr, symIdx);
        unsigned sym = articleSymbols.at(symIdx);
        vec.push_back(sym);
        if (sym >= 0x10000) {
            if (sym >= 0x10040) {
                unsigned startIdx = bstr->read(BitLength(len));
                unsigned len = sym - 0x1003d;
                res += res.substr(startIdx, len);
                vec.push_back(startIdx);
            } else {
                unsigned startIdx = bstr->read(BitLength(prefix.length()));
                unsigned len = sym - 0xfffd;
                res += prefix.substr(startIdx, len);
                vec.push_back(startIdx);
            }
        } else {
            res += (char16_t)sym;
        }
    }
    return true;
}

void UserDictionaryDecoder::Read(IBitStream *bstr) {
    int len = bstr->read(32);
    _prefix = readUnicodeString(bstr, len, true);
    _articleSymbols = readSymbols(bstr);
    _headingSymbols = readSymbols(bstr);
    _ltArticles.Read(*bstr);
    _ltHeadings.Read(*bstr);
    _ltPrefixLengths.Read(*bstr);
    _ltPostfixLengths.Read(*bstr);
    _huffman1Number = bstr->read(32);
    _huffman2Number = bstr->read(32);
}

void UserDictionaryDecoder::DecodeHeading(IBitStream *bstr, unsigned len, std::u16string &res) {
    res.clear();
    unsigned symIdx;
    for (size_t i = 0; i < len; i++) {
        _ltHeadings.Decode(*bstr, symIdx);
        unsigned sym = _headingSymbols.at(symIdx);
        assert(sym <= 0xffff);
        res += (char16_t)sym;
    }
}

bool UserDictionaryDecoder::DecodeArticle(IBitStream *bstr, std::u16string &res) {
    return DecodeArticle(bstr, res, _prefix, _ltArticles, _articleSymbols);
}

bool UserDictionaryDecoder::DecodePrefixLen(IBitStream& bstr, unsigned &len) {
    return _ltPrefixLengths.Decode(bstr, len);
}

bool UserDictionaryDecoder::DecodePostfixLen(IBitStream &bstr, unsigned &len) {
    return _ltPostfixLengths.Decode(bstr, len);
}

bool UserDictionaryDecoder::ReadReference1(IBitStream &bstr, unsigned &reference) {
    return readReference(bstr, reference, _huffman1Number);
}

bool UserDictionaryDecoder::ReadReference2(IBitStream &bstr, unsigned &reference) {
    return readReference(bstr, reference, _huffman2Number);
}

std::u16string UserDictionaryDecoder::Prefix() {
    return _prefix;
}

}
