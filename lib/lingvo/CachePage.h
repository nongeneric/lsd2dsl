#pragma once

#include "ArticleHeading.h"

#include <vector>
#include <string>

namespace lingvo {

class IDictionaryDecoder;

// CCacheNodePage and CCacheLeafPage
class CachePage {
    bool _isLeaf;
    unsigned _number;
    unsigned _prev;
    unsigned _next;
    unsigned _parent;
    unsigned _headingsCount;
public:
    bool loadHeader(common::IBitStream& bstr);
    bool isLeaf() const;
    unsigned number() const;
    unsigned prev() const;
    unsigned next() const;
    unsigned parent() const;
    unsigned headingsCount() const; // or prefixes in case of the CCacheNodePage
    unsigned firstChild() const;
};

struct NodePageBody {
    unsigned firstChild;
    std::vector<std::u16string> prefixes;
};

std::vector<ArticleHeading> parseLeafPageBody(common::IBitStream& bstr, IDictionaryDecoder& decoder, unsigned count, std::u16string knownPrefix);
NodePageBody parseNodePageBody(common::IBitStream& bstr, IDictionaryDecoder& decoder, unsigned count);

}
