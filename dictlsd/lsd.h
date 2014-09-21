#pragma once

#include "BitStream.h"
#include "ArticleHeading.h"
#include <string>
#include <vector>
#include <memory>

namespace dictlsd {

#pragma pack(1)
struct LSDHeader {
    char magic[8];
    uint32_t version;
    uint32_t unk;
    uint32_t checksum;
    uint32_t entriesCount;
    uint32_t annotationOffset;
    uint32_t dictionaryEncoderOffset;
    uint32_t articlesOffset;
    uint32_t pagesOffset;
    uint32_t unk1;
    uint16_t unk2;
    uint16_t unk3;
    uint16_t sourceLanguage;
    uint16_t targetLanguage;
};
#pragma pack()

struct OverlayHeading {
    std::u16string name;
    uint32_t offset;
    uint32_t unk2;
    uint32_t inflatedSize;
    uint32_t streamSize;
};

class DictionaryReader;
class LSDOverlayReader;

class LSDDictionary {
    IBitStream* _bstr;
    std::unique_ptr<DictionaryReader> _reader;
    std::unique_ptr<LSDOverlayReader> _overlayReader;
public:
    LSDDictionary(IBitStream* bitstream);
    std::u16string name() const;
    std::u16string annotation() const;
    std::vector<char> icon() const;
    LSDHeader header() const;
    std::vector<ArticleHeading> readHeadings();
    std::u16string readArticle(unsigned reference) const;
    std::vector<OverlayHeading> readOverlayHeadings();
    std::vector<uint8_t> readOverlayEntry(OverlayHeading const& heading);
    ~LSDDictionary();
};

}
