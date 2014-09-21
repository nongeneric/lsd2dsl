#pragma once

#include "lsd.h"
#include "BitStream.h"

#include <string>
#include <memory>
#include <vector>

namespace dictlsd {

class IDictionaryDecoder;
class DictionaryReader {
    LSDHeader _header;
    IBitStream* _bstr;    
    std::unique_ptr<IDictionaryDecoder> _decoder;
    unsigned _articlesCount; // not really it seems
    std::u16string _name;
    unsigned _pagesEnd;
    unsigned _overlayData;
    std::vector<char> _icon;
    void readOverlay();
public:
    DictionaryReader(IBitStream* bitstream);
    std::u16string name();
    std::u16string prefix();
    std::u16string annotation();
    unsigned pagesCount() const;
    unsigned overlayHeadingsOffset() const;
    unsigned overlayDataOffset() const;
    std::vector<char> icon() const;
    std::u16string decodeArticle(IBitStream& bstr, unsigned reference);
    IDictionaryDecoder* decoder();
    LSDHeader* header();
};

}
