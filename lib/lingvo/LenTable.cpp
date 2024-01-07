#include "LenTable.h"
#include "common/BitStream.h"
#include "tools.h"
#include "common/bformat.h"

#include <assert.h>
#include <algorithm>
#include <limits>
#include <map>
#include <stack>

namespace lingvo {

int tryGetPairWeight(const std::vector<IdxWeightPair> &pairs, size_t idx) {
    if (idx < pairs.size())
        return pairs.at(idx).weight;
    return std::numeric_limits<int>::max();
}

int tryGetVec16Weight(const std::vector<HuffmanNode> &nodes, size_t idx) {
    if (idx < nodes.size())
        return nodes.at(idx).weight;
    return std::numeric_limits<int>::max();
}

unsigned LenTable::GetMaxLen() const {
    assert(nodes.size() > 0);
    assert(symidx2nodeidx.size() > 0);
    int maxlen = 0;
    for (int nodeIdx : symidx2nodeidx) {
        int len = 1;
        int parent = nodes.at(nodeIdx).parent;
        while (parent != -1) {
            len++;
            parent = nodes.at(parent).parent;
        }
        maxlen = std::max(maxlen, len);
    }
    return maxlen;
}

bool LenTable::placeSymidx(int symIdx, int nodeIdx, int len) {
    assert(len > 0);
    if (len == 1) { // time to place
        if (nodes.at(nodeIdx).left == 0) {
            nodes.at(nodeIdx).left = -1 - symIdx;
            symidx2nodeidx[symIdx] = nodeIdx;
            return true;
        }
        if (nodes.at(nodeIdx).right == 0) {
            nodes.at(nodeIdx).right = -1 - symIdx;
            symidx2nodeidx[symIdx] = nodeIdx;
            return true;
        }
        return false;
    }
    if (nodes.at(nodeIdx).left == 0) {
        nodes.at(nextNodePosition) = {0,0,nodeIdx,-1};
        nodes.at(nodeIdx).left = ++nextNodePosition;
    }
    if (nodes.at(nodeIdx).left > 0) {
        if (placeSymidx(symIdx, nodes.at(nodeIdx).left - 1, len - 1))
            return true;
    }
    if (nodes.at(nodeIdx).right == 0) {
        nodes.at(nextNodePosition) = {0,0,nodeIdx,-1};
        nodes.at(nodeIdx).right = ++nextNodePosition;
    }
    if (nodes.at(nodeIdx).right > 0) {
        if (placeSymidx(symIdx, nodes.at(nodeIdx).right - 1, len - 1))
            return true;
    }
    return false;
}

void LenTable::Read(common::IBitStream &bitstr) {
    symidx2nodeidx.clear();
    nodes.clear();

    int count = bitstr.read(32);
    int bitsPerLen = bitstr.read(8);
    int idxBitSize = BitLength(count);

    symidx2nodeidx.resize(count);
    for (unsigned& nodeIdx : symidx2nodeidx) {
        nodeIdx = -1; // in case the root has a leaf as a child
    }
    nodes.resize(count - 1);
    int rootIdx = nodes.size() - 1;
    nodes.at(rootIdx) = {0,0,-1,-1};
    nextNodePosition = 0;
    for (int i = 0; i < count; ++i) {
        int symidx = bitstr.read(idxBitSize);
        int len = bitstr.read(bitsPerLen);
        placeSymidx(symidx, rootIdx, len);
    }
}

std::string dumpNode(int childIdx, int nodeIdx, std::string edgelabel) {
    if (childIdx != 0) {
        if (childIdx > 0) {
            return bformat("%d -> %d [label=\" %s \"]\n",
                           childIdx - 1, nodeIdx, edgelabel);
        } else {
            return bformat("sym_%d -> %d [label=\" %s \"]\n",
                           -1 - childIdx, nodeIdx, edgelabel);
        }
    }
    return "";
}

std::string LenTable::DumpDot() const {
    std::string res = "digraph G {\n";
    for (size_t nodeIdx = 0; nodeIdx < nodes.size(); ++nodeIdx) {
        HuffmanNode const& node = nodes[nodeIdx];
        res += dumpNode(node.left, nodeIdx, "0");
        res += dumpNode(node.right, nodeIdx, "1");
    }
    return res + "}";
}

int LenTable::Decode(common::IBitStream &bitstr, unsigned &symIdx) const {
    const HuffmanNode* node = &nodes.back();
    int len = 0;
    for (;;) {
        len++;
        int bit = bitstr.read(1);
        if (bit) { // right
            if (node->right < 0) { // leaf
                symIdx = -1 - node->right;
                return len;
            }
            node = &nodes.at(node->right - 1);
        } else { // left
            if (node->left < 0) { // leaf
                symIdx = -1 - node->left;
                return len;
            }
            node = &nodes.at(node->left - 1);
        }
    }
    // unreachable
}

}
