#include "AbbreviationDictionaryDecoder.h"

#include "common/BitStream.h"
#include "UserDictionaryDecoder.h"
#include "tools.h"

namespace lingvo {

std::u16string readXoredPrefix(common::IBitStream* bstr, int len) {
    std::u16string res;
    for (int i = 0; i < len; i++) {
        char32_t symbol = bstr->read(16) ^ 0x879A;
        res += symbol;
    }
    return res;
}

std::vector<char32_t> readXoredSymbols(common::IBitStream* bstr) {
    std::vector<char32_t> res;
    int len = bstr->read(32);
    int bitsPerSymbol = bstr->read(8);
    for (int i = 0; i < len; i++) {
        char32_t symbol = bstr->read(bitsPerSymbol) ^ 0x1325;
        res.push_back(symbol);
    }
    return res;
}

void AbbreviationDictionaryDecoder::Read(common::IBitStream *bstr){
    int len = bstr->read(32);
    _prefix = readXoredPrefix(bstr, len);
    _articleSymbols = readXoredSymbols(bstr);
    _headingSymbols = readXoredSymbols(bstr);
    _ltArticles.Read(*bstr);
    _ltHeadings.Read(*bstr);

    _ltPrefixLengths.Read(*bstr);
    _ltPostfixLengths.Read(*bstr);

    _huffman1Number = bstr->read(32);
    _huffman2Number = bstr->read(32);
}

void AbbreviationDictionaryDecoder::DecodeHeading(common::IBitStream *bstr, unsigned len, std::u16string &res) {
    res.clear();
    unsigned symIdx;
    for (size_t i = 0; i < len; i++) {
        _ltHeadings.Decode(*bstr, symIdx);
        unsigned sym = _headingSymbols.at(symIdx);
        assert(sym <= 0xffff);
        res += (char16_t)sym;
    }
}

bool AbbreviationDictionaryDecoder::DecodeArticle(common::IBitStream *bstr, std::u16string &res) {
    return UserDictionaryDecoder::DecodeArticle(bstr, res, _prefix, _ltArticles, _articleSymbols);
}

bool AbbreviationDictionaryDecoder::DecodePrefixLen(common::IBitStream &bstr, unsigned &len) {
    return _ltPrefixLengths.Decode(bstr, len);
}

bool AbbreviationDictionaryDecoder::DecodePostfixLen(common::IBitStream &bstr, unsigned &len) {
    return _ltPostfixLengths.Decode(bstr, len);
}

bool AbbreviationDictionaryDecoder::ReadReference1(common::IBitStream &bstr, unsigned &reference) {
    return readReference(bstr, reference, _huffman1Number);
}

bool AbbreviationDictionaryDecoder::ReadReference2(common::IBitStream &bstr, unsigned &reference) {
    return readReference(bstr, reference, _huffman2Number);
}

std::u16string AbbreviationDictionaryDecoder::Prefix() {
    return _prefix;
}

}
