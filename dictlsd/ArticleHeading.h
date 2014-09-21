#pragma once

#include "BitStream.h"
#include <string>
#include <vector>

namespace dictlsd {

class IDictionaryDecoder;
class ArticleHeading {
    std::u16string _text;
    unsigned _reference;
    std::vector<std::pair<int,std::u16string>> _extensions;
public:
    bool Load(IDictionaryDecoder& decoder,
              IBitStream& bstr,
              std::u16string& knownPrefix);
    std::u16string text() const;
    std::u16string extText() const;
    unsigned articleReference() const;
};

}
