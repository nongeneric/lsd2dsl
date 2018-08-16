#pragma once

#include "IDictionaryDecoder.h"
#include "LenTable.h"

#include <vector>
#include <string>

namespace dictlsd {

class SystemDictionaryDecoder : public IDictionaryDecoder {
    std::u16string _prefix;
    std::vector<char32_t> _articleSymbols;
    std::vector<char32_t> _headingSymbols;
    LenTable _ltArticles;
    LenTable _ltHeadings;
    LenTable _ltPrefixLengths;
    LenTable _ltPostfixLengths;
    unsigned _huffman1Number;
    unsigned _huffman2Number;
    bool _xoring;
public:
    SystemDictionaryDecoder(bool xoring);
    static bool DecodeArticle(
        IBitStream *bstr,
        std::u16string &res,
        std::u16string const& prefix,
        bool xoring,
        LenTable& ltArticles,
        std::vector<char32_t>& articleSymbols);
    virtual void Read(IBitStream* bstr) override;
    virtual void DecodeHeading(IBitStream* bstr, unsigned len, std::u16string& body) override;
    virtual bool DecodeArticle(IBitStream* bstr, std::u16string& body) override;
    virtual bool DecodePrefixLen(IBitStream& bstr, unsigned& len) override;
    virtual bool DecodePostfixLen(IBitStream& bstr, unsigned& len) override;
    virtual bool ReadReference1(IBitStream& bstr, unsigned& reference) override;
    virtual bool ReadReference2(IBitStream& bstr, unsigned& reference) override;
    virtual std::u16string Prefix() override;
};

}
