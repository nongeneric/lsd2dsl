#pragma once

#include "BitStream.h"
#include <string>

namespace dictlsd {

class IDictionaryDecoder {
public:
    virtual ~IDictionaryDecoder();
    virtual void Read(IBitStream* bstr) = 0;
    virtual void DecodeHeading(IBitStream* bstr, unsigned len, std::u16string& body) = 0;
    virtual bool DecodeArticle(IBitStream* bstr, std::u16string& body) = 0;
    virtual bool DecodePrefixLen(IBitStream& bstr, unsigned& len) = 0;
    virtual bool DecodePostfixLen(IBitStream& bstr, unsigned& len) = 0;
    virtual bool ReadReference1(IBitStream& bstr, unsigned& reference) = 0;
    virtual bool ReadReference2(IBitStream& bstr, unsigned& reference) = 0;
    virtual std::u16string Prefix() = 0;
};

}
