#include "CachePage.h"
#include "BitStream.h"
#include "IDictionaryDecoder.h"

namespace dictlsd {

bool CachePage::loadHeader(IBitStream &bstr) {
    _isLeaf = bstr.read(1);
    _number = bstr.read(16);
    _prev = bstr.read(16);
    _parent = bstr.read(16);
    _next = bstr.read(16);
    _headingsCount = bstr.read(16);
    bstr.toNearestByte();
    return true;
}

bool CachePage::isLeaf() const {
    return _isLeaf;
}

unsigned CachePage::number() const {
    return _number;
}

unsigned CachePage::prev() const {
    return _prev;
}

unsigned CachePage::next() const {
    return _next;
}

unsigned CachePage::parent() const {
    return _parent;
}

unsigned CachePage::headingsCount() const {
    return _headingsCount;
}

NodePageBody parseNodePageBody(
        IBitStream &bstr,
        IDictionaryDecoder& decoder,
        unsigned count)
{
    NodePageBody res;
    decoder.ReadReference1(bstr, res.firstChild);
    for (unsigned i = 0; i < count; ++i) {
        if (i == count - 1) {
            res.prefixes.push_back(u"");
            continue;
        }
        unsigned prefixLen;
        decoder.DecodePrefixLen(bstr, prefixLen);
        unsigned postfixLen;
        decoder.DecodePostfixLen(bstr, postfixLen);
        std::u16string heading;
        decoder.DecodeHeading(&bstr, postfixLen, heading);
        res.prefixes.push_back(heading);
    }
    return res;
}


std::vector<ArticleHeading> parseLeafPageBody(
        IBitStream &bstr,
        IDictionaryDecoder &decoder,
        unsigned count,
        std::u16string knownPrefix)
{
    std::vector<ArticleHeading> headings;
    for (unsigned i = 0; i < count; ++i) {
        ArticleHeading heading;
        heading.Load(decoder, bstr, knownPrefix);
        knownPrefix = heading.text();
        headings.push_back(heading);
    }
    return headings;
}

}
