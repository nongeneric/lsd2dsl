#pragma once

#include "lib/lsd/BitStream.h"
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <set>

namespace duden {

enum class HicEntryType {
    Plain = 1,
    Reference = 2,
    Plain3 = 3,
    Range = 4,
    Person = 6,
    VariantWith = 7,
    VariantWithout = 8,
    Variant = 10,
    Unknown11 = 11
};

struct HicPage;

struct HicLeaf {
    std::string heading;
    HicEntryType type;
    uint32_t textOffset;
};

struct HicNode {
    std::string heading;
    std::shared_ptr<HicPage> page;
    int count;
    int delta;
    uint32_t hicOffset;
};

using HicEntry = std::variant<HicLeaf, HicNode>;

struct HicPage {
    uint32_t offset;
    std::vector<HicEntry> entries;
};

struct HicFile {
    std::string name;
    int version;
    std::shared_ptr<HicPage> root;
};

struct FsiEntry {
    std::string name;
    uint32_t offset;
    uint32_t size;

    bool operator<(const FsiEntry& other) const {
        return std::tie(offset, size) < std::tie(other.offset, other.size);
    }
};

std::vector<HicEntry> parseHicNode45(dictlsd::IRandomAccessStream* stream);
std::vector<HicEntry> parseHicNode6(dictlsd::IRandomAccessStream* stream);
void decodeBofBlock(const void* blockData,
                    uint32_t blockSize,
                    std::vector<char>& output);
std::vector<uint32_t> parseIndex(dictlsd::IRandomAccessStream* stream);
std::vector<FsiEntry> parseFsiBlock(dictlsd::IRandomAccessStream* stream);
std::set<FsiEntry> parseFsiFile(dictlsd::IRandomAccessStream* stream);
HicFile parseHicFile(dictlsd::IRandomAccessStream* stream);
std::string dudenToUtf8(std::string str);
std::string win1252toUtf8(std::string str);

struct HeadingGroup {
    std::vector<std::string> headings;
    int32_t articleSize = -1;
};

std::map<int32_t, HeadingGroup> groupHicEntries(std::vector<HicLeaf> entries);

} // namespace duden
