#include "ArticleHeading.h"

#include "IDictionaryDecoder.h"
#include "BitStream.h"
#include <assert.h>

namespace dictlsd {

bool ArticleHeading::Load(
        IDictionaryDecoder &decoder,
        IBitStream &bstr,
        std::u16string &knownPrefix)
{
    _text.clear();

    unsigned prefixLen;
    decoder.DecodePrefixLen(bstr, prefixLen);
    unsigned postfixLen;
    decoder.DecodePostfixLen(bstr, postfixLen);
    decoder.DecodeHeading(&bstr, postfixLen, _text);
    decoder.ReadReference2(bstr, _reference);
    _text = knownPrefix.substr(0, prefixLen) + _text;
    if (bstr.read(1)) {
        std::vector<std::pair<int,char16_t>> chars;
        unsigned len = bstr.read(8);
        for (unsigned i = 0; i < len; ++i) {
            int idx = bstr.read(8);
            char16_t chr = bstr.read(16);
            chars.emplace_back(idx, chr);
        }
        std::u16string str;
        int idx = chars[0].first;
        for (size_t i = 0; i < chars.size(); ++i) {
            str += chars[i].second;
            if (i == chars.size() - 1 || chars[i + 1].first != chars[i].first + 1) {
                _extensions.emplace_back(idx, str);
                idx = chars[i + 1].first;
                str = u"";
            }
        }
    }
    return true;
}

std::u16string ArticleHeading::text() const {
    return _text;
}

std::u16string ArticleHeading::extText() const {
    if (_extensions.empty()) {
        return _text;
    }
    std::u16string extText = _text;
    int offset = 0;
    for (size_t i = 0; i < _extensions.size(); ++i) {
        std::u16string str = _extensions[i].second;
        bool addBraces = str != u"\\";
        if (addBraces) {
            str = u"{" + str + u"}";
        }
        extText.insert(_extensions[i].first + offset, str);
        if (addBraces) {
            offset += 2;
        }
    }
    return extText;
}

unsigned ArticleHeading::articleReference() const {
    return _reference;
}

}
