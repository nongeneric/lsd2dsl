#pragma once

#include "BitStream.h"
#include <string>
#include <deque>

namespace dictlsd {

struct ExtPair {
    unsigned char idx;
    char16_t chr;
};

class IDictionaryDecoder;
class ArticleHeading {
    std::deque<ExtPair> _pairs;
    std::u16string _text;
    bool _hasExtText;
    std::u16string _extText;
    unsigned _reference;
public:
    ArticleHeading();
    bool Load(IDictionaryDecoder& decoder,
              IBitStream& bstr,
              std::u16string& knownPrefix);
    std::u16string text() const;
    std::u16string extText();
    unsigned articleReference() const;
};

}
