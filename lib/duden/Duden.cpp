#include "Duden.h"

#include "unzip/inflate.h"
#include "zlib.h"
#include <QTextCodec>
#include <boost/algorithm/string.hpp>
#include <regex>
#include <tuple>
#include "assert.h"

namespace duden {

#pragma pack(1)
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
        utf.push_back(ch);

        if (utf.back() == '}') {
            sref = false;
        }

        auto size = utf.size();
        if (size >= 3) {
            if (utf[size - 3] == '\\' && utf[size - 2] == 'S' && utf[size - 1] == '{') {
                sref = true;
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
    auto current = block[0].heading;
    for (auto it = begin(block) + 1; it != end(block); ++it) {
        uint8_t ch = it->heading[0];
        if (ch < 0x20) {
            it->heading.replace(0, 1, current.substr(0, ch));
        }
        current = it->heading;
    }
}

static void parseHicNodeHeadings(dictlsd::IRandomAccessStream* stream, std::vector<HicEntry>& block) {
    for (auto& entry : block) {
        readLine(stream, entry.heading, '\0');
    }

    block.erase(std::remove_if(begin(block), end(block), [&](auto& entry) {
        return !entry.isLeaf;
    }), end(block));

    decodeHeadingPrefixes(block);

    for (auto& entry : block) {
        entry.heading = dudenToUtf8(entry.heading);
    }
}

std::vector<HicEntry> parseHicNode6(dictlsd::IRandomAccessStream* stream) {
    auto count = read8(stream);
    assert(count);
    std::vector<HicEntry> block(count);
    for (auto& entry : block) {
        auto raw = read32(stream);
        auto type = read8(stream);
        entry.isLeaf = (raw & 1) == 0;
        entry.type = static_cast<HicEntryType>(type >> 4);
        entry.textOffset = (raw >> 1) - 1;
        if (!entry.isLeaf) {
            read32(stream);
        }
    }
    parseHicNodeHeadings(stream, block);
    return block;
}

std::vector<HicEntry> parseHicNode45(dictlsd::IRandomAccessStream* stream) {
    auto count = read8(stream);
    assert(count);
    std::vector<HicEntry> block(count);
    for (auto& entry : block) {
        auto raw = read32(stream);
        entry.isLeaf = (raw & 1) == 0;
        entry.type = static_cast<HicEntryType>((raw >> 1) & 0xf);
        entry.textOffset = (raw >> 5) - 1;
        if (!entry.isLeaf) {
            read32(stream);
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
            if ((offset == 0 && str.empty()))
                break;
            auto [name, size] = parseFsiEntry(str);
            res.push_back({name, offset, size});
            if (last)
                break;
            read8(stream);
        }
    }
    return res;
}

std::vector<FsiEntry> parseFsiFile(dictlsd::IRandomAccessStream* stream) {
    const auto blockSize = 0x400;
    std::vector<FsiEntry> res;
    stream->seek(0x12);
    auto blockCount = read16(stream);
    for (auto i = 1; i <= blockCount; ++i) {
        stream->seek(i * blockSize);
        auto block = parseFsiBlock(stream);
        std::copy(begin(block), end(block), std::back_inserter(res));
    }
    return res;
}

HicFile parseHicFile(dictlsd::IRandomAccessStream *stream) {
    std::string magic(0x22, 0);
    stream->readSome(&magic[0], magic.size());
    if (magic != "compressed PC-Bibliothek Hierarchy")
        throw std::runtime_error("not a HIC file");
    read8(stream);
    auto version = read8(stream);
    if (version <= 3)
        throw std::runtime_error("unsupported version");
    Hic5Header header;
    stream->readSome(reinterpret_cast<char*>(&header), sizeof header);
    std::string name(header.namelen - 1, 0);
    stream->readSome(&name[0], name.size());
    read8(stream);
    HicFile hicFile{name, version, {}};

    for (auto i = 0u; i < header.blockCount; ++i) {
        auto nodeSize = read16(stream);
        auto curPos = stream->tell();
        auto entries = version >= 6 ? parseHicNode6(stream) : parseHicNode45(stream);
        if (curPos + nodeSize != stream->tell())
            throw std::runtime_error("failed to parse a HIC node");
        std::copy(begin(entries), end(entries), std::back_inserter(hicFile.entries));
    }

    if (header.headingCount != hicFile.entries.size())
        throw std::runtime_error("heading count inconsistency");

    return hicFile;
}

static std::tuple<std::string, int32_t> parseHeading(const std::string& heading) {
    static std::regex rx(R"(^(.*?)( \$\$\$\$\s+\-?\d+\s(\d+)\s\-?\d+)?$)");
    std::smatch m;
    if (!std::regex_match(heading, m, rx))
        throw std::runtime_error("can't parse heading");
    int64_t offset = -1;
    if (m[3].length()) {
        offset = std::stoi(m[3]) - 1;
    }
    return {m[1], offset};
}

std::map<int32_t, HeadingGroup> groupHicEntries(std::vector<HicEntry> entries) {
    std::map<int32_t, HeadingGroup> groups;
    for (const auto& entry : entries) {
        auto [name, offset] = parseHeading(entry.heading);
        offset = offset == -1 ? entry.textOffset : offset;
        if (entry.type == HicEntryType::Variant)
            continue;
        groups[offset].headings.emplace_back(std::move(name));
    }

    entries.erase(std::remove_if(begin(entries), end(entries), [](auto& entry) {
        return entry.type != HicEntryType::Plain
            && entry.type != HicEntryType::Variant;
    }), end(entries));

    std::map<int32_t, int32_t> sizes;
    if (!entries.empty()) {
        for (auto i = 0u; i < entries.size() - 1; ++i) {
            auto size = entries[i + 1].textOffset - entries[i].textOffset;
            sizes[entries[i].textOffset] = size;
        }
    }

    for (auto& group : groups) {
        auto size = sizes.find(group.first);
        if (size != end(sizes)) {
            group.second.articleSize = size->second;
        }
        std::sort(begin(group.second.headings), end(group.second.headings));
    }

    return groups;
}

} // namespace duden
