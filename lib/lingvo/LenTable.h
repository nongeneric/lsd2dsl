#pragma once

#include "common/BitStream.h"
#include <vector>
#include <string>
#include <stdint.h>

namespace lingvo {

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
    void Store(common::IBitStream& bitstr) const;
    void Read(common::IBitStream& bitstr);
    std::string DumpDot() const;
    int Decode(common::IBitStream& bitstr, unsigned& symIdx) const;
    bool placeSymidx(int symIdx, int nodeIdx, int len);
};

}
