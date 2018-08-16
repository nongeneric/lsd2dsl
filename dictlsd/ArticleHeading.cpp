#include "ArticleHeading.h"

#include "IDictionaryDecoder.h"
#include "BitStream.h"
#include <assert.h>
#include <map>
#include <algorithm>
#include <list>
#include <memory>

namespace dictlsd {

void ArticleHeading::makeCharsFromPairs(std::deque<ExtPair>& pairs, std::u16string const& text) {
    std::deque<char16_t> textDeque(begin(text), end(text));
    size_t idx = 0;
    _chars.reserve(pairs.size());
    auto nextChar = [&](char16_t& chr) {
        if (!pairs.empty() && pairs.front().idx == idx) {
            chr = pairs.front().chr;
            pairs.pop_front();
            return false;
        }
        chr = textDeque.front();
        textDeque.pop_front();
        return true;
    };

    while (!textDeque.empty() || !pairs.empty()) {
        CharInfo info;
        info.escaped = false;
        info.sorted = nextChar(info.chr);
        if (info.chr == '\\') {
            idx++;
            info.sorted = nextChar(info.chr);
            info.escaped = true;
        }
        _chars.push_back(info);
        idx++;
    }
}

bool ArticleHeading::Load(
        IDictionaryDecoder &decoder,
        IBitStream &bstr,
        std::u16string &knownPrefix)
{
    std::u16string text;
    std::deque<ExtPair> pairs;

    unsigned prefixLen;
    decoder.DecodePrefixLen(bstr, prefixLen);
    unsigned postfixLen;
    decoder.DecodePostfixLen(bstr, postfixLen);
    decoder.DecodeHeading(&bstr, postfixLen, text);
    decoder.ReadReference2(bstr, _reference);
    knownPrefix = knownPrefix.substr(0, prefixLen) + text;
    if (bstr.read(1)) {
        unsigned len = bstr.read(8);
        for (unsigned i = 0; i < len; ++i) {
            unsigned char idx = bstr.read(8);
            char16_t chr = bstr.read(16);
            pairs.push_back({idx, chr});
        }
    }
    makeCharsFromPairs(pairs, knownPrefix);
    return true;
}

std::u16string ArticleHeading::text() const {
    std::u16string text;
    for (auto& info : _chars) {
        if (info.sorted) {
            text += info.chr;
        }
    }
    return text;
}

std::u16string ArticleHeading::dslText() {
    std::u16string extText;
    extText.clear();
    bool group = false;
    for (CharInfo& info : _chars) {
        if (group && info.sorted) {
            extText += u"}";
            group = false;
        } else if (!group && !info.sorted) {
            extText += u"{";
            group = true;
        }
        if (info.escaped)
            extText += '\\';
        extText += info.chr;
    }
    if (group) {
        extText += u"}";
    }
    return extText;
}

unsigned ArticleHeading::articleReference() const {
    return _reference;
}

std::function<bool(CharInfo const&, CharInfo const&)> makePred(bool ignoreSortedness) {
    return [ignoreSortedness](CharInfo const& l, CharInfo const& r) {
        return l.escaped == r.escaped
            && l.chr == r.chr
            && (l.sorted == r.sorted || ignoreSortedness);
    };
}

CharVec::const_iterator find(CharVec::const_iterator first, CharVec::const_iterator last, CharVec const& subrange) {
    return std::search(first, last, begin(subrange), end(subrange), makePred(false));
}

bool isUnsorted(CharInfo const& info) {
    return !info.sorted;
}

bool compare(CharVec const& a, CharVec const& b, bool ignoreSortedness) {
    if (a.size() != b.size())
        return false;
    return std::equal(begin(a), end(a), begin(b), makePred(ignoreSortedness));
}

void append(CharVec& vec, CharVec const& another) {
    std::copy(begin(another), end(another), std::back_inserter(vec));
}

bool matchB(CharVec const& chars, CharVec& left, CharVec& middle, CharVec& right) {
    auto m = find(begin(chars), end(chars), { { false, false, '(' } });
    auto r = find(m, end(chars), { { false, false, ')' } } );
    if (m == end(chars) || r == end(chars))
        return false;
    left = CharVec(begin(chars), m);
    middle = CharVec(m + 1, r);
    ++m;
    ++r;
    right = CharVec(r, end(chars));
    return true;
}

bool matchA(CharVec const& chars, CharVec& left, CharVec& middle, CharVec& right) {
    bool isB = matchB(chars, left, middle, right);
    return isB && std::all_of(begin(middle), end(middle), isUnsorted);
}

bool matchCD(CharVec const& chars, CharVec& left, CharVec& middle, CharVec& right, bool isFirstSpaceSorted) {
    auto m = find(begin(chars), end(chars), {
        { isFirstSpaceSorted, false, ' ' }, { false, false, '(' }
    });
    auto r = find(m, end(chars), {
        { false, false, ')' }
    });
    if (m == end(chars) || r == end(chars))
        return false;
    if (r + 1 != end(chars))
        return false;
    left = CharVec(begin(chars), m);
    std::advance(m, 2);
    middle = CharVec(m, r);
    right.clear();
    return true;
}

bool matchC(CharVec const& chars, CharVec& left, CharVec& middle, CharVec& right) {
    bool match = matchCD(chars, left, middle, right, false);
    return match && std::all_of(begin(middle), end(middle), isUnsorted);
}

bool matchD(CharVec const& chars, CharVec& left, CharVec& middle, CharVec& right) {
    return matchCD(chars, left, middle, right, true);
}

bool matchEF(CharVec const& chars, CharVec& left, CharVec& middle, CharVec& right, bool isLastSpaceSorted) {
    auto m = find(begin(chars), end(chars), {
        { true, false, ' ' }, { false, false, '(' }
    });
    auto r = find(m, end(chars), {
        { false, false, ')' }, { isLastSpaceSorted, false, ' ' }
    });
    if (m == end(chars) || r == end(chars))
        return false;
    left = CharVec(begin(chars), m);
    std::advance(m, 2);
    middle = CharVec(m, r);
    std::advance(r, 2);
    right = CharVec(r, end(chars));
    return true;
}

bool matchE(CharVec const& chars, CharVec& left, CharVec& middle, CharVec& right) {
    bool match = matchEF(chars, left, middle, right, false);
    return match && std::all_of(begin(middle), end(middle), isUnsorted);
}

bool matchF(CharVec const& chars, CharVec& left, CharVec& middle, CharVec& right) {
    return matchEF(chars, left, middle, right, true);
}

bool tryCollapse(ArticleHeading& variant1,
                 ArticleHeading& variant2,
                 ArticleHeading& collapsed,
                 CharVec const& beforeMiddle,
                 CharVec const& afterMiddle,
                 Matcher matcherA,
                 Matcher matcherB)
{
    variant1.dslText();
    variant2.dslText();
    CharVec const& chars1 = variant1._chars;
    CharVec const& chars2 = variant2._chars;
    CharVec aleft, amiddle, aright;
    CharVec bleft, bmiddle, bright;
    bool match = false;
    if (matcherA(chars1, aleft, amiddle, aright)) {
        match = matcherB(chars2, bleft, bmiddle, bright);
    } else if (matcherB(chars1, bleft, bmiddle, bright)){
        match = matcherA(chars2, aleft, amiddle, aright);
    }
    if (match
        && compare(aleft, bleft, false)
        && compare(amiddle, bmiddle, true)
        && compare(aright, bright, false))
    {
        collapsed = variant1;
        collapsed._chars.clear();
        append(collapsed._chars, bleft);
        append(collapsed._chars, beforeMiddle);
        append(collapsed._chars, bmiddle);
        append(collapsed._chars, afterMiddle);
        append(collapsed._chars, bright);
        return true;
    }
    return false;
}

bool tryCollapseAB(ArticleHeading& variant1, ArticleHeading& variant2, ArticleHeading& collapsed) {
    static CharVec beforeMiddle { { true, false, '(' } };
    static CharVec afterMiddle { { true, false, ')' } };
    return tryCollapse(variant1, variant2, collapsed, beforeMiddle, afterMiddle, matchA, matchB);
}

bool tryCollapseCD(ArticleHeading& variant1, ArticleHeading& variant2, ArticleHeading& collapsed) {
    static CharVec beforeMiddle {
        { true, false, ' ' },
        { true, false, '(' }
    };
    static CharVec afterMiddle { { true, false, ')' } };
    return tryCollapse(variant1, variant2, collapsed, beforeMiddle, afterMiddle, matchC, matchD);
}

bool tryCollapseEF(ArticleHeading& variant1, ArticleHeading& variant2, ArticleHeading& collapsed) {
    static CharVec beforeMiddle {
        { true, false, ' ' },
        { true, false, '(' }
    };
    static CharVec afterMiddle {
        { true, false, ')' },
        { true, false, ' ' }
    };
    return tryCollapse(variant1, variant2, collapsed, beforeMiddle, afterMiddle, matchE, matchF);
}

std::vector<ArticleHeading>::iterator tryCollapsePair(
        std::vector<ArticleHeading>::iterator first,
        std::vector<ArticleHeading>::iterator last)
{
    for (auto i = first; i != last; ++i) {
        for (auto j = std::next(i); j != last; ++j) {
            if (tryCollapseAB(*i, *j, *i) ||
                tryCollapseCD(*i, *j, *i) ||
                tryCollapseEF(*i, *j, *i)) {
                return j;
            }
        }
    }
    return last;
}

void foreachReferenceSet(std::vector<ArticleHeading>& groupedHeadings,
                         std::function<void(ArticleHeadingIter, ArticleHeadingIter)> func,
                         bool dontGroup)
{
    for (auto it = begin(groupedHeadings); it != end(groupedHeadings);) {
        auto first = it;
        auto ref = first->articleReference();
        if (dontGroup) {
            ++it;
        } else {
            it = std::find_if(it, end(groupedHeadings), [ref](auto& h) {
                return h.articleReference() != ref;
            });
        }
        func(first, it);
    }
}

void collapseVariants(std::vector<ArticleHeading>& headings) {
    groupHeadingsByReference(headings);
    // collapse adjacent variant headings if possible
    std::vector<bool> toRemove(headings.size(), false);
    foreachReferenceSet(headings, [&](auto first, auto last) {
        if (std::distance(first, last) > 1) {
            for (;;) {
                auto j = tryCollapsePair(first, last);
                if (j == last)
                    break;
                toRemove[std::distance(begin(headings), j)] = true;
            }
        }
    });
    // compress the vector of headings and remove the collapsed tail
    std::vector<ArticleHeading> compressed;
    std::copy_if(begin(headings), end(headings), std::back_inserter(compressed), [&](auto& h) {
        auto idx = &h - &headings[0];
        return !toRemove[idx];
    });
    std::swap(headings, compressed);
}

bool CharInfo::operator==(CharInfo const& other) const {
    return sorted == other.sorted
        && escaped == other.escaped
        && chr == other.chr;
}

void groupHeadingsByReference(std::vector<ArticleHeading>& vec) {
    std::map<unsigned, std::vector<size_t>> map;
    for (auto it = begin(vec); it != end(vec); ++it) {
        map[it->articleReference()].push_back(std::distance(begin(vec), it));
    }
    std::vector<ArticleHeading> res;
    for (auto it = begin(vec); it != end(vec); ++it) {
        auto ref = it->articleReference();
        auto pair = map.find(ref);
        for (auto j = begin(pair->second); j != end(pair->second); ++j) {
            res.push_back(vec[*j]);
        }
        pair->second.clear();
    }
    std::swap(vec, res);
}

}
