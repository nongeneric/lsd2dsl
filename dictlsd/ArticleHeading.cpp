#include "ArticleHeading.h"

#include "IDictionaryDecoder.h"
#include "BitStream.h"
#include <assert.h>
#include <map>
#include <algorithm>
#include <list>

namespace dictlsd {

void ArticleHeading::makeExtTextFromChars() {
    _extText.clear();
    bool group = false;
    for (CharInfo& info : _chars) {
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
}

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

std::u16string ArticleHeading::extText() {
    if (_hasExtText)
        return _extText;

    if (_pairs.empty())
        return _text;

    std::deque<char16_t> text(begin(_text), end(_text));
    size_t idx = 0;
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
        _chars.push_back(info);
        idx++;
    }

    makeExtTextFromChars();

    _hasExtText = true;
    return _extText;
}

unsigned ArticleHeading::articleReference() const {
    return _reference;
}

/*      Unsorted parts
 *
 * Each characted in a LSD heading has a flag indicating if it's 'sorted' or 'unsorted'.
 * The unsorted characters aren't used in sortings and searching. Basically they become
 * transparent for the indexing mechanism and only visible while presented to the user
 * together with an article. Let's annotate each character with an 's' or 'u' to indicate
 * this distinction.
 *
 * In the DSL format curly brackets are used to denote the sortedness of a heading's part.
 *
 * The heading 'Some{thing}' would be encoded as
 *
 *   Something
 *   ssssuuuuu
 *
 * And it's name would be just 'Some', while its extended name would be 'Something'
 * There can be any number of unsorted characters in a heading. At any position.
 *
 * To correctly decompile an LSD heading (where each character is either sorted or unsorted)
 * to a DSL heading, it is required to group adjacent characters in groups by their
 * sortedness, and then enclose each such group in parentheses. This process is rather
 * straightforward. The special case to look for is the slash (/), which is encoded
 * as an unsorted character, but is used to escape the subsequent characted (either sorted
 * or unsorted) and thus requires a special handling.
 *
 *      Optional parts (variants)
 *
 * The DSL format has a mechanism for generating several headings with optional parts
 * out of a single heading. By enclosing a part of a heading in parentheses, it is possible
 * to generate all possible combinations of the heading.
 *
 * The heading 'aa(bb)cc' would be expanded into two different headings
 *
 *   aa{(bb)}cc  and  aa{(}bb{)}cc
 *   ss uuuu ss       ss u ss u ss
 *
 * For more such parts, more headings would be generated. It is 2 headings for 1 part,
 * 4 headings for 2 parts, 9 for 3 and so on.
 *
 * There exists a pattern which helps to combine two headings produced with the variant
 * encoding. The pattern looks like this 'u(' denotes an unsorted parenthesis):
 *
 *   (A)  ??? u(   u*n u) ???   ->   ??? s( s|u*n s) ???
 *   (B)  ??? u( s|u*n u) ???
 *
 * An optional part can contain an unsorted part as well, so this pattern allow for a
 * combination of sorted/unsorted parts in an optional part (denoted as 's|u*n').
 *
 * This pattern becomes insufficient in the presence of spaces, adjacent to the
 * parentheses. Here are two examples of such headings (the upper case is used
 * to denote spaces):
 *
 *   abc (123)
 *
 *   abc{ (123)}    and   abc {(}123{)}
 *   sss Uuuuuu           sssS u sss u
 *
 *   bbb (123) z
 *
 *   bbb {(123) }z  and   bbb {(}123{)} z
 *   sssS uuuuuU s        sssS u sss u Ss
 *
 * Another two patterns account for these special cases.
 *
 *   (C) ??? Uu(   u*n u)  ->  ??? Ss( s|u*n s)
 *   (D) ??? Su( s|u*n u)
 *
 *   (E) ??? Su(   u*n u)U ???  ->  ??? Ss( s|u*n s)S ???
 *   (F) ??? Su( s|u*n u)S ???
 *
 * The headings that can't be collapsed using one of these three rules are left as is.
 *
 */

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
    variant1.extText();
    variant2.extText();
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
        && compare(aright, bright, false)) {
        collapsed = variant1;
        collapsed._chars.clear();
        append(collapsed._chars, bleft);
        append(collapsed._chars, beforeMiddle);
        append(collapsed._chars, bmiddle);
        append(collapsed._chars, afterMiddle);
        append(collapsed._chars, bright);
        collapsed.makeExtTextFromChars();
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
        && chr == chr;
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
