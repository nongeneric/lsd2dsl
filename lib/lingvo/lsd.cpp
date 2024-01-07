#include "lsd.h"
#include "ArticleHeading.h"
#include "DictionaryReader.h"
#include "IDictionaryDecoder.h"
#include "common/BitStream.h"
#include "CachePage.h"
#include "LSDOverlayReader.h"

#include <algorithm>

namespace lingvo {

std::vector<ArticleHeading> collectHeadingFromPage(common::IBitStream& bstr, DictionaryReader& reader, unsigned pageNumber) {
    std::vector<ArticleHeading> res;
    bstr.seek(reader.header().pagesOffset + 512 * pageNumber);
    CachePage page;
    page.loadHeader(bstr);
    if (page.isLeaf()) {
        std::u16string prefix;
        for (size_t idx = 0; idx < page.headingsCount(); ++idx) {
            ArticleHeading h;
            h.Load(*reader.decoder(), bstr, prefix);
            res.push_back(h);
        }
    }
    return res;
}

LSDDictionary::LSDDictionary(common::IBitStream *bitstream)
    : _bstr(bitstream)
{
    _reader.reset(new DictionaryReader(_bstr));
    _overlayReader.reset(new LSDOverlayReader(_bstr, _reader.get()));
}

std::vector<ArticleHeading> LSDDictionary::readHeadings() const {
    std::vector<ArticleHeading> headings;
    for (size_t i = 0; i < _reader->pagesCount(); ++i) {
        auto page = collectHeadingFromPage(*_bstr, *_reader, i);
        copy(begin(page), end(page), back_inserter(headings));
    }
    return headings;
}

std::u16string LSDDictionary::readArticle(unsigned reference) const {
    return _reader->decodeArticle(*_bstr, reference);
}

std::vector<OverlayHeading> LSDDictionary::readOverlayHeadings() const {
    return _overlayReader->readHeadings();
}

std::vector<uint8_t> LSDDictionary::readOverlayEntry(OverlayHeading const& heading) const {
    return _overlayReader->readEntry(heading);
}

bool LSDDictionary::supported() const {
    return _reader->supported();
}

std::u16string LSDDictionary::name() const {
    return _reader->name();
}

std::u16string LSDDictionary::annotation() const {
    return _reader->annotation();
}

std::vector<unsigned char> const& LSDDictionary::icon() const {
    return _reader->icon();
}

LSDHeader const& LSDDictionary::header() const {
    return _reader->header();
}

LSDDictionary::~LSDDictionary() { }

}
