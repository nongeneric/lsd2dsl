#pragma once

#include "lsd.h"

#include <string>
#include <memory>
#include <vector>
#include <stdexcept>

namespace lingvo {

class NotLSDException : public std::exception {
public:
    virtual const char *what() const noexcept;
};

class DictionaryReader {
    LSDHeader _header;
    common::IBitStream* _bstr;
    mutable std::unique_ptr<IDictionaryDecoder> _decoder;
    unsigned _articlesCount; // not really it seems
    std::u16string _name;
    unsigned _pagesEnd;
    unsigned _overlayData;
    std::vector<unsigned char> _icon;
    void readOverlay();
    bool _isSupported;
    mutable bool _decoderLoaded;
    void loadDecoder() const;
public:
    DictionaryReader(common::IBitStream* bitstream);
    bool supported() const;
    std::u16string name() const;
    std::u16string prefix() const;
    std::u16string annotation() const;
    unsigned pagesCount() const;
    unsigned overlayHeadingsOffset() const;
    unsigned overlayDataOffset() const;
    std::vector<unsigned char> const& icon() const;
    std::u16string decodeArticle(common::IBitStream& bstr, unsigned reference);
    IDictionaryDecoder* decoder();
    LSDHeader const& header() const;
};

}
