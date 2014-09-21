#pragma once

#include "BitStream.h"
#include <vector>
#include <string>
#include <stdint.h>

namespace dictlsd {

struct IdxWeightPair {
    unsigned idx;
    unsigned weight;
};

struct SymInfo {
    int symidx;
    int len;
    int code;
};

struct HuffmanNode {
    int left;
    int right;
    int parent;
    int weight;
};

class LenTable {
public:
    std::vector<HuffmanNode> nodes;
    std::vector<unsigned> symidx2nodeidx;
    std::vector<unsigned> bits;
    int nextNodePosition;
    unsigned GetMaxLen() const;
    void Store(IBitStream& bitstr) const;
    void Read(IBitStream& bitstr);
    std::string DumpDot() const;
    int Decode(IBitStream& bitstr, unsigned& symIdx) const;
    bool placeSymidx(int symIdx, int nodeIdx, int len);
};

}
