#pragma once

#include "LenTable.h"
#include "IDictionaryDecoder.h"

#include <string>
#include <vector>

namespace lingvo {

class AbbreviationDictionaryDecoder : public IDictionaryDecoder {
    std::u16string _prefix;
    std::vector<char32_t> _articleSymbols;
    std::vector<char32_t> _headingSymbols;
    LenTable _ltArticles;
    LenTable _ltHeadings;
    LenTable _ltPrefixLengths;
    LenTable _ltPostfixLengths;
    unsigned _huffman1Number;
    unsigned _huffman2Number;
public:
    virtual void Read(common::IBitStream* bstr) override;
    virtual void DecodeHeading(common::IBitStream* bstr, unsigned len, std::u16string& body) override;
    virtual bool DecodeArticle(common::IBitStream* bstr, std::u16string& body) override;
    virtual bool DecodePrefixLen(common::IBitStream& bstr, unsigned& len) override;
    virtual bool DecodePostfixLen(common::IBitStream& bstr, unsigned& len) override;
    virtual bool ReadReference1(common::IBitStream& bstr, unsigned& reference) override;
    virtual bool ReadReference2(common::IBitStream& bstr, unsigned& reference) override;
    virtual std::u16string Prefix() override;
};

}
