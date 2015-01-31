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
 *   ??? u(   u*n u) ???   ->   ??? s( s|u*n s) ???
 *   ??? u( s|u*n u) ???
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
 *   sssS uuuuuU s        sssS u sss u Us
 *
 * Another two patterns account for these special cases.
 *
 *   ??? Su(   u*n u)  ->  ??? Ss( s|u*n s)
 *   ??? Uu( s|u*n u)
 *
 *   ??? S u(   u*n u)U ???  ->  ??? S s( s|u*n s) S ???
 *   ??? S u( s|u*n u)S ???
 *
 * The headings that can't be collapsed by these three rules are left as is.
 *
 */

typedef std::vector<CharInfo> CharVec;

void splitVariantChars(CharVec& chars,
                       CharVec& left,
                       CharVec& middle,
                       CharVec& right)
{
    auto m = std::find_if(begin(chars), end(chars), [](CharInfo i) { return !i.escaped && !i.sorted && i.chr == '('; });
    auto r = std::find_if(m, end(chars), [](CharInfo i) { return !i.escaped && !i.sorted && i.chr == ')'; });
    if (r != end(chars))
        ++r;
    left = CharVec(begin(chars), m);
    middle = CharVec(m, r);
    right = CharVec(r, end(chars));
}

bool tryCollapse(ArticleHeading& variant1, ArticleHeading& variant2, ArticleHeading& collapsed) {
    if (variant1.extText().size() < 3 ||
        variant2.extText().size() < 3)
        return false;
    auto& v1chars = variant1._chars;
    auto& v2chars = variant2._chars;
    CharVec v1left, v1middle, v1right;
    CharVec v2left, v2middle, v2right;
    splitVariantChars(v1chars, v1left, v1middle, v1right);
    splitVariantChars(v2chars, v2left, v2middle, v2right);
    if (v1middle.empty() || v2middle.empty())
        return false;
    if (v1left != v2left || v1right != v2right)
        return false;
    if (!(v1middle.front() == v2middle.front() &&
        v1middle.back() == v2middle.back()))
        return false;
    for (size_t i = 1; i < v1middle.size() - 1; ++i) {
        if (v1middle.at(i).escaped != v2middle.at(i).escaped ||
            v1middle.at(i).chr != v2middle.at(i).chr)
            return false;
    }
    auto isUnsorted = [](CharInfo info) { return !info.sorted; };
    bool v1middleIsUnsorted = std::all_of(begin(v1middle), end(v1middle), isUnsorted);
    bool v2middleIsUnsorted = std::all_of(begin(v2middle), end(v2middle), isUnsorted);
    if (!(v1middleIsUnsorted || v2middleIsUnsorted))
        return false;
    collapsed = v1middleIsUnsorted ? variant2 : variant1;
    collapsed._chars.at(v1left.size()).sorted = true;
    collapsed._chars.at(v1left.size() + v1middle.size() - 1).sorted = true;
    collapsed.makeExtTextFromChars();
    return true;
}

std::vector<ArticleHeading>::iterator tryCollapsePair(
        std::vector<ArticleHeading>::iterator first,
        std::vector<ArticleHeading>::iterator last)
{
    for (auto i = first; i != last; ++i) {
        for (auto j = std::next(i); j != last; ++j) {
            if (tryCollapse(*i, *j, *i)) {
                return j;
            }
        }
    }
    return last;
}


void foreachReferenceSet(std::vector<ArticleHeading>& groupedHeadings,
                         std::function<void(ArticleHeadingIter, ArticleHeadingIter)> func)
{
    for (auto it = begin(groupedHeadings); it != end(groupedHeadings);) {
        auto first = it;
        auto ref = first->articleReference();
        it = std::find_if(it, end(groupedHeadings), [ref](auto& h) {
            return h.articleReference() != ref;
        });
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
                toRemove[std::distance(j, end(headings))] = true;
            }
        }
    });
    // compress the vector of headings and remove the collapsed tail
    auto last = std::stable_partition(begin(headings), end(headings), [&](auto& h) {
        auto idx = &h - &headings[0];
        return !toRemove[idx];
    });
    headings.erase(last, end(headings));
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
