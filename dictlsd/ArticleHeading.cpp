#include "ArticleHeading.h"

#include "IDictionaryDecoder.h"
#include "BitStream.h"
#include <assert.h>

namespace dictlsd {

ArticleHeading::ArticleHeading()
    : _hasExtText(false) { }

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
        unsigned len = bstr.read(8);
        for (unsigned i = 0; i < len; ++i) {
            unsigned char idx = bstr.read(8);
            char16_t chr = bstr.read(16);
            _pairs.push_back({idx, chr});
        }
    }
    return true;
}

std::u16string ArticleHeading::text() const {
    return _text;
}

struct CharInfo {
    bool sorted;
    bool escaped;
    char16_t chr;
};

std::u16string ArticleHeading::extText() {
    if (_pairs.empty())
        return _text;

    if (_hasExtText)
        return _extText;

    std::deque<char16_t> text(begin(_text), end(_text));
    size_t idx = 0;
    std::vector<CharInfo> chars;
    auto nextChar = [&](char16_t& chr) {
        if (!_pairs.empty() && _pairs.front().idx == idx) {
            chr = _pairs.front().chr;
            _pairs.pop_front();
            return false;
        }
        chr = text.front();
        text.pop_front();
        return true;
    };

    while (!text.empty() || !_pairs.empty()) {
        CharInfo info;
        info.escaped = false;
        info.sorted = nextChar(info.chr);
        if (info.chr == '\\') {
            idx++;
            info.sorted = nextChar(info.chr);
            info.escaped = true;
        }
        chars.push_back(info);
        idx++;
    }

    bool group = false;
    for (CharInfo& info : chars) {
        if (group && info.sorted) {
            _extText += u"}";
            group = false;
        } else if (!group && !info.sorted) {
            _extText += u"{";
            group = true;
        }
        if (info.escaped)
            _extText += '\\';
        _extText += info.chr;
    }
    if (group) {
        _extText += u"}";
    }

    _hasExtText = true;
    return _extText;
}

unsigned ArticleHeading::articleReference() const {
    return _reference;
}

}
