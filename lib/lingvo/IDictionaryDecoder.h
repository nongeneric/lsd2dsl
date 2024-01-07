#pragma once

#include "common/BitStream.h"
#include <string>

namespace lingvo {

class IDictionaryDecoder {
public:
    virtual ~IDictionaryDecoder();
    virtual void Read(common::IBitStream* bstr) = 0;
    virtual void DecodeHeading(common::IBitStream* bstr, unsigned len, std::u16string& body) = 0;
    virtual bool DecodeArticle(common::IBitStream* bstr, std::u16string& body) = 0;
    virtual bool DecodePrefixLen(common::IBitStream& bstr, unsigned& len) = 0;
    virtual bool DecodePostfixLen(common::IBitStream& bstr, unsigned& len) = 0;
    virtual bool ReadReference1(common::IBitStream& bstr, unsigned& reference) = 0;
    virtual bool ReadReference2(common::IBitStream& bstr, unsigned& reference) = 0;
    virtual std::u16string Prefix() = 0;
};

}
