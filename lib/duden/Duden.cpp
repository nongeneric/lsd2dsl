#include "Duden.h"

#include "unzip/inflate.h"
#include "zlib.h"
#include <QTextCodec>
#include <boost/algorithm/string.hpp>
#include <regex>
#include <tuple>
#include <unordered_map>
#include "assert.h"
#include "lib/common/overloaded.h"
#include "lib/common/bformat.h"

namespace duden {

#pragma pack(1)
struct Hic3Header {
    uint32_t headingCount;
    uint32_t blockCount;
    uint16_t unk7;
    uint16_t unk8;
    uint32_t unk9;
    uint16_t unk10;
    uint8_t unk11;
    uint8_t namelen;
};
struct Hic4Header {
    uint32_t unk0;
    uint32_t unk1;
    uint16_t unk3;
    uint32_t unk4;
    uint32_t headingCount;
    uint32_t blockCount;
    uint16_t unk7;
    uint16_t unk8;
    uint32_t unk9;
    uint16_t unk10;
    uint8_t unk11;
    uint8_t namelen;
};
struct Hic5Header {
    uint32_t unk0;
    uint32_t unk1;
    uint32_t unk2;
    uint16_t unk3;
    uint32_t unk4;
    uint32_t headingCount;
    uint32_t blockCount;
    uint16_t unk7;
    uint16_t unk8;
    uint32_t unk9;
    uint16_t unk10;
    uint8_t unk11;
    uint8_t namelen;
};
#pragma pack()

void decodeBofBlock(const void* blockData,
                    uint32_t blockSize,
                    std::vector<char>& output) {
    static std::vector<char> buffer(32 << 10);
    unsigned outputSize = buffer.size();
    auto res = duden_inflate(blockData, blockSize, &buffer[0], &outputSize);
    if (res)
        throw std::runtime_error("inflate failed");
    output.assign(begin(buffer), begin(buffer) + outputSize);
}

const uint16_t dudenTable[] = {
    0x2992, 0x2694, 0x0000, 0x0294, 0x00AE, 0x2655, 0x26AE, 0x26AD, 0x007E,
    0x0000, 0x020D, 0x020E, 0x020F, 0x0210, 0x00E6, 0x00E7, 0x00F0, 0x00F8,
    0x0127, 0x014B, 0x0153, 0x03B2, 0x03B8, 0x0111, 0x0180, 0x021C, 0x0195,
    0x021E, 0x021F, 0x0220, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066,
    0x0067, 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078,
    0x0079, 0x007A, 0x023B, 0x023C, 0x023D, 0x023E, 0x023F, 0x0240, 0x0241,
    0x0242, 0x0152, 0x0153
};

static uint32_t dudenCharToUtf(uint32_t ch) {
    switch (ch) {
        case 0x25FFu: return 0xA0;
        case 0x25FEu: return 0x2012;
        case 0x25FDu: return 0x2014;
    }
    if ((uint16_t)(ch - 0x203) > 0x41u) {
        if (ch == 0x36E)
            return 0x35C;
        else
            return ch != 0x36F ? ch : 0;
    } else {
        ch -= 0x203;
        assert(ch < sizeof(dudenTable) / sizeof(uint16_t));
        return dudenTable[ch];
    }
}

static uint16_t win1252toUtf(char ch) {
    static auto codec = QTextCodec::codecForName("Windows-1252");
    auto qstr = codec->toUnicode(&ch, 1);
    return qstr[0].unicode();
}

std::string win1252toUtf8(std::string str) {
    static auto codec = QTextCodec::codecForName("Windows-1252");
    auto utf8 = codec->toUnicode(str.data(), str.size()).toUtf8();
    return {utf8.begin(), utf8.end()};
}

std::string dudenToUtf8(std::string str) {
    std::vector<uint32_t> utf;
    auto i = 0u;

    auto next = [&] {
        if (i >= str.size())
            throw std::runtime_error("bad encoding, expected more bytes");
        return (uint8_t)str[i++];
    };

    bool sref = false;

    while (i < str.size()) {
        auto first = next();
        uint32_t ch = first;
        if (!sref) {
            if (first >= 0xa0) {
                ch = (ch << 8) | next();
                if (first >= 0xf6) {
                    ch = (ch << 8) | next();
                    if (first >= 0xfc) {
                        ch = (ch << 8) | next();
                    }
                }
            }
            if (ch >= 0xf600)
                throw std::runtime_error("bad encoding");
            if (ch < 0xa0) {
            } else if (ch < 0xa100) {
                ch &= 0xff;
            } else {
                auto c = uint8_t(ch - 0x21);
                if (c > 0x5e) {
                    c -= 0x21;
                }
                ch = 0xbe * (uint16_t(ch + 0x5edf) >> 8) + c + 0x100;
            }
            ch = dudenCharToUtf(ch);
            if (ch < 256) {
                ch = win1252toUtf(ch);
            }
        }

        if (ch) {
            utf.push_back(ch);
        }

        if (utf.back() == '}') {
            sref = false;
        }

        auto size = utf.size();
        if (size >= 3) {
            auto isSorW = utf[size - 2] == 'S' || utf[size - 2] == 'w';
            if (utf[size - 3] == '\\' && isSorW && utf[size - 1] == '{') {
                sref = true;
            }
        }
        if (size >= 2) {
            if (utf[size - 2] == '@' && utf[size - 1] == 'C') {
                auto c = next();
                if (c == '%') {
                    utf.push_back(c);
                } else {
                    utf.push_back(win1252toUtf(c));
                    while (i < str.size()) {
                        auto c = next();
                        utf.push_back(win1252toUtf(c));
                        if (c == '\n')
                            break;
                    }
                }
            }
        }
    }
    static auto codec = QTextCodec::codecForName("UTF32LE");
    auto utf8 = codec->toUnicode((const char*)&utf[0], utf.size() * sizeof(uint32_t)).toUtf8();
    return {utf8.begin(), utf8.end()};
}

static void decodeHeadingPrefixes(std::vector<HicEntry>& block) {
    if (block.empty())
        return;
    std::string current;
    for (auto it = begin(block); it != end(block); ++it) {
        auto& heading = std::visit(overloaded{[=](auto& typed) -> std::string& { return typed.heading; }}, *it);
        uint8_t ch = heading[0];
        if (ch < 0x20) {
            heading.replace(0, 1, current.substr(0, ch));
        }
        current = heading;
    }
}

static void parseHicNodeHeadings(dictlsd::IRandomAccessStream* stream, std::vector<HicEntry>& block) {
    for (auto& entry : block) {
        std::visit(overloaded{[=](auto& typed) { readLine(stream, typed.heading, '\0'); }}, entry);
    }

    decodeHeadingPrefixes(block);

    for (auto& entry : block) {
        std::visit(overloaded{[=](auto& typed) { typed.heading = dudenToUtf8(typed.heading); }}, entry);
    }
}

std::vector<HicEntry> parseHicNode6(dictlsd::IRandomAccessStream* stream) {
    auto count = read8(stream);
    assert(count);
    std::vector<HicEntry> block;
    for (auto i = 0u; i < count; ++i) {
        auto raw = read32(stream);
        auto type = read8(stream);
        auto isLeaf = (raw & 1) == 0;
        if (isLeaf) {
            HicLeaf leaf;
            leaf.textOffset = (raw >> 1) - 1;
            leaf.type = static_cast<HicEntryType>(type >> 4);
            block.push_back(leaf);
        } else {
            HicNode node;
            node.delta = read32(stream);
            node.count = type;
            node.hicOffset = raw >> 1;
            block.push_back(std::move(node));
        }
    }
    parseHicNodeHeadings(stream, block);
    return block;
}

std::vector<HicEntry> parseHicNode45(dictlsd::IRandomAccessStream* stream) {
    auto count = read8(stream);
    std::vector<HicEntry> block;
    for (auto i = 0u; i < count; ++i) {
        auto raw = read32(stream);
        auto isLeaf = (raw & 1) == 0;
        if (isLeaf) {
            HicLeaf leaf;
            leaf.textOffset = (raw >> 5) - 1;
            leaf.type = static_cast<HicEntryType>((raw >> 1) & 0xf);
            block.push_back(leaf);
        } else {
            HicNode node;
            node.delta = read32(stream);
            node.count = (raw >> 1) & 0xf;
            node.hicOffset = raw >> 9;
            block.push_back(std::move(node));
        }
    }
    parseHicNodeHeadings(stream, block);
    return block;
}

std::vector<uint32_t> parseIndex(dictlsd::IRandomAccessStream* stream) {
    std::vector<uint32_t> res;
    for (;;) {
        uint32_t index;
        if (stream->readSome(&index, sizeof index) != sizeof index)
            break;
        res.push_back(index);
    }
    return res;
}

static std::tuple<std::string, uint32_t> parseFsiEntry(std::string raw) {
    std::smatch m;
    if (!std::regex_match(raw, m, std::regex("^(.+?);(\\d+)$")))
        throw std::runtime_error("parsing error");
    return {m[1], static_cast<uint32_t>(std::stol(m[2]))};
}

static std::tuple<bool, std::string> parseFsiString(dictlsd::IRandomAccessStream *stream) {
    std::string res;
    for (;;) {
        auto ch = read8(stream);
        if (ch == 0xa1)
            return {true, res};
        if (!ch)
            return {false, res};
        res += ch;
    }
    assert(false);
    return {};
}

std::vector<FsiEntry> parseFsiBlock(dictlsd::IRandomAccessStream* stream) {
    auto type = read16(stream);
    assert(type == 0xc || type == 0xb);
    read32(stream);
    auto rawCount = read16(stream);
    std::vector<FsiEntry> res;
    if (type == 0xc) {
        stream->seek(stream->tell() + 7);
        for (auto i = 0u; i < rawCount * 2; ++i) {
            auto offset = read32(stream);
            auto [last, str] = parseFsiString(stream);
            if (str.empty()) {
                if (offset == 0)
                    break;
                read8(stream);
                std::tie(last, str) = parseFsiString(stream);
            }
            if (offset == 0 && str.empty())
                break;
            auto [name, size] = parseFsiEntry(str);
            name = win1252toUtf8(name);
            res.push_back({name, offset, size});
            if (last || peek32(stream) == 0xa1a1a1a1)
                break;
            read8(stream);
        }
    }
    return res;
}

std::set<FsiEntry> parseFsiFile(dictlsd::IRandomAccessStream* stream) {
    const auto blockSize = 0x400;
    std::set<FsiEntry> res;
    stream->seek(0x12);
    auto blockCount = read16(stream);
    for (auto i = 1; i <= blockCount; ++i) {
        stream->seek(i * blockSize);
        auto block = parseFsiBlock(stream);
        std::copy(begin(block), end(block), std::inserter(res, begin(res)));
    }
    return res;
}

template <class T>
std::tuple<uint32_t, uint32_t, uint8_t> readHeader(dictlsd::IRandomAccessStream *stream) {
    T header;
    stream->readSome(reinterpret_cast<char*>(&header), sizeof header);
    return {header.headingCount, header.blockCount, header.namelen};
}

HicFile parseHicFile(dictlsd::IRandomAccessStream *stream) {
    std::string magic(0x22, 0);
    stream->readSome(&magic[0], magic.size());
    if (magic != "compressed PC-Bibliothek Hierarchy")
        throw std::runtime_error("not a HIC file");
    read8(stream);
    auto version = read8(stream);
    if (version < 3)
        throw std::runtime_error("unsupported version");

    auto [headingCount, blockCount, namelen] =
        version == 3 ? readHeader<Hic3Header>(stream) :
        version == 4 ? readHeader<Hic4Header>(stream) :
        readHeader<Hic5Header>(stream);
    std::string name(namelen - 1, 0);
    stream->readSome(&name[0], name.size());
    read8(stream);
    HicFile hicFile{name, version, {}};

    std::unordered_map<uint32_t, std::shared_ptr<HicPage>> pages;
    for (auto i = 0u; i < blockCount; ++i) {
        auto curPos = stream->tell();
        auto nodeSize = read16(stream);
        auto entries = version >= 6 ? parseHicNode6(stream) : parseHicNode45(stream);
        stream->seek(curPos + nodeSize + sizeof(nodeSize));

        auto page = std::make_shared<HicPage>();
        page->offset = curPos;
        page->entries = std::move(entries);
        pages[curPos] = page;
        if (i == 0) {
            hicFile.root = page;
        }
    }

    unsigned leafCount = 0;
    for (auto& [_, page] : pages) {
        for (auto& entry : page->entries) {
            std::visit(overloaded{[&](HicLeaf&) { leafCount++; },
                                  [&](HicNode& node) {
                                      auto pageIt = pages.find(node.hicOffset);
                                      if (pageIt == end(pages))
                                          throw std::runtime_error("hic is misformed");
                                      node.page = pageIt->second;
                                  }},
                       entry);
        }
    }

    assert(headingCount == leafCount);

    return hicFile;
}

struct ParsedHeading {
    std::string name;
    int64_t offset;
};

static std::optional<ParsedHeading> parseHeading(const std::string& heading) {
    static std::regex rx(R"(^(.*?)( \$\$\$\$\s+(\-?\d+)\s(\d+)\s\-?\d+(\s\-?\d+)?)?$)");
    std::smatch m;
    if (!std::regex_match(heading, m, rx))
        throw std::runtime_error("can't parse heading");
    if (m[5].length())
        return {};
    int64_t offset = -1;
    if (m[4].length()) {
        offset = std::stoi(m[4]) - 1;
    }
    return ParsedHeading{m[1], offset};
}

std::map<int32_t, HeadingGroup> groupHicEntries(std::vector<HicLeaf> entries) {
    std::map<int32_t, HeadingGroup> groups;
    for (const auto& entry : entries) {
        auto heading = parseHeading(entry.heading);
        if (!heading)
            continue;
        if (heading->offset == -1) {
            heading->offset = entry.textOffset;
        }
        if (entry.type == HicEntryType::Variant)
            continue;
        groups[heading->offset].headings.emplace_back(std::move(heading->name));
    }

    entries.erase(std::remove_if(begin(entries), end(entries), [](auto& entry) {
        return entry.type != HicEntryType::Plain
            && entry.type != HicEntryType::Plain3
            && entry.type != HicEntryType::Variant;
    }), end(entries));

    for (auto it = begin(groups); it != end(groups); ++it) {
        auto next = std::next(it);
        if (next != end(groups)) {
            it->second.articleSize = next->first - it->first;
        }
        std::sort(begin(it->second.headings), end(it->second.headings));
    }

    return groups;
}

} // namespace duden
