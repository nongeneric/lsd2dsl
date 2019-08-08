#include <gtest/gtest.h>

#include "lib/duden/Archive.h"
#include "lib/duden/Duden.h"
#include "lib/duden/InfFile.h"
#include "lib/duden/Dictionary.h"
#include "lib/duden/LdFile.h"
#include "lib/duden/text/TextRun.h"
#include "lib/duden/text/Table.h"
#include "lib/duden/text/Parser.h"
#include "lib/duden/text/Printers.h"
#include "lib/duden/text/Reference.h"
#include "lib/duden/HtmlRenderer.h"
#include "lib/duden/TableRenderer.h"
#include "lib/common/bformat.h"
#include "lib/common/filesystem.h"
#include "test-utils.h"
#include <QApplication>
#include "zlib.h"
#include <boost/algorithm/string.hpp>
#include <map>

using namespace duden;
using namespace dictlsd;
using namespace std::literals;

class duden_qt : public ::testing::Test {
    int _c = 0;
    QApplication _app;

public:
    duden_qt() : _app(_c, nullptr) { }
    ~duden_qt() = default;
};

TEST(duden, HicNodeTest1) {
    FileStream stream("duden_testfiles/HicNode99");

    auto block = parseHicNode45(&stream);
    auto at = [&](int i) { return std::get<HicLeaf>(block[i]); };

    ASSERT_EQ(13, block.size());
    ASSERT_EQ(u8"Inhalt", at(0).heading);
    ASSERT_EQ(u8"Vorwort", at(1).heading);
    ASSERT_EQ(u8"Impressum", at(2).heading);
    ASSERT_EQ(u8"Zur Einrichtung des Wörterverzeichnisses", at(3).heading);
    ASSERT_EQ(u8"Fremdwörter: Bedrohung oder Bereicherung", at(4).heading);
    ASSERT_EQ(u8"Ein Fremdwort – was ist das?", at(5).heading);
    ASSERT_EQ(
        u8"Fremdes Wort im deutschen Satz: Schreibung, Aussprache und Grammatik",
        at(6).heading);
    ASSERT_EQ(u8"Fremdwörter in Zahlen", at(7).heading);
    ASSERT_EQ(u8"Fremdwörter – eine Stilfrage", at(10).heading);

    ASSERT_EQ(0, at(0).textOffset);
    ASSERT_EQ(HicEntryType::Plain, at(0).type);
    ASSERT_EQ(0x2c2, at(1).textOffset);
    ASSERT_EQ(HicEntryType::Plain, at(1).type);
}

TEST(duden, HicNodeTest2) {
    FileStream stream("duden_testfiles/HicNode10c5");

    auto block = parseHicNode45(&stream);
    auto at = [&](int i) { return std::get<HicLeaf>(block[i]); };

    ASSERT_EQ(20, block.size());
    ASSERT_EQ(u8"absent", at(1).heading);
    ASSERT_EQ(u8"absentia", at(2).heading);
    ASSERT_EQ(u8"absentieren", at(3).heading);
    ASSERT_EQ(u8"Absolventin", at(15).heading);
}

TEST(duden, HicNodeTest3) {
    FileStream stream("duden_testfiles/block_hic_v6");

    auto block = parseHicNode6(&stream);
    auto at = [&](int i) { return std::get<HicLeaf>(block[i]); };

    ASSERT_EQ(18, block.size());
    ASSERT_EQ(u8"Abd ar-Rahman Putra", at(0).heading);
    ASSERT_EQ(HicEntryType::Plain, at(0).type);
    ASSERT_EQ(0x51286, at(0).textOffset);

    ASSERT_EQ(u8"Abderemane", at(17).heading);
    ASSERT_EQ(HicEntryType::Plain, at(17).type);
    ASSERT_EQ(0x52a02, at(17).textOffset);
}

TEST(duden, HicNodeTest4) {
    FileStream stream("duden_testfiles/block_2847_heading_encoding");

    auto block = parseHicNode6(&stream);
    auto at = [&](int i) { return std::get<HicLeaf>(block[i]); };

    ASSERT_EQ(18, block.size());
    ASSERT_EQ(u8"8-bit-Zeichensatz", at(0).heading);
    ASSERT_EQ(u8"a.", at(10).heading);
    ASSERT_EQ(u8"à", at(11).heading);
}

TEST(duden, HicNodeTest_ParseNonLeafs) {
    FileStream stream("duden_testfiles/HicNode6a");

    auto block = parseHicNode45(&stream);
    auto at = [&](int i) { return std::get<HicNode>(block[i]); };

    ASSERT_EQ(2, block.size());
    EXPECT_EQ("Zusätze", at(0).heading);
    EXPECT_EQ(13, at(0).count);
    EXPECT_EQ(-67283, at(0).delta);
    EXPECT_EQ(151, at(0).hicOffset);

    EXPECT_EQ("Wörterverzeichnis", at(1).heading);
    EXPECT_EQ(10, at(1).count);
    EXPECT_EQ(-7912591, at(1).delta);
    EXPECT_EQ(667, at(1).hicOffset);
}

TEST(duden, ParseIndex) {
    uint32_t buf[] = {0x13131414, 0x15161516, 0x33331111, 0x11223344};
    InMemoryStream stream(buf, sizeof(buf));

    auto index = parseIndex(&stream);

    ASSERT_EQ(4, index.size());
    ASSERT_EQ(0x13131414, index[0]);
    ASSERT_EQ(0x15161516, index[1]);
    ASSERT_EQ(0x33331111, index[2]);
    ASSERT_EQ(0x11223344, index[3]);
}

TEST(duden, ParseSingleItemFsiBlock) {
    FileStream stream("duden_testfiles/fsiSingleItemBlock");

    auto entries = parseFsiBlock(&stream);

    ASSERT_EQ(1, entries.size());
    ASSERT_EQ(u8"EURO.BMP", entries[0].name);
    ASSERT_EQ(0, entries[0].offset);
    ASSERT_EQ(1318, entries[0].size);
}

TEST(duden, ParseSingleItemFsiBlockUnicode) {
    FileStream stream("duden_testfiles/fsiSingleItemBlockUnicode");

    auto entries = parseFsiBlock(&stream);

    ASSERT_EQ(1, entries.size());
    ASSERT_EQ("EURä.BMP", entries[0].name);
    ASSERT_EQ(0, entries[0].offset);
    ASSERT_EQ(1318, entries[0].size);
}

TEST(duden, ParseCFsiBlock) {
    FileStream stream("duden_testfiles/fsiCBlock");
    auto entries = parseFsiBlock(&stream);
    ASSERT_EQ(40, entries.size());
    ASSERT_EQ("110187Z.BMP", entries[0].name);
    ASSERT_EQ(0, entries[0].offset);
    ASSERT_EQ(827314, entries[0].size);

    ASSERT_EQ("110226Z.BMP", entries[39].name);
    ASSERT_EQ(0x016f2d5a, entries[39].offset);
    ASSERT_EQ(1327350, entries[39].size);
}

TEST(duden, ParseCFsiBlock2) {
    FileStream stream("duden_testfiles/fsiCBlock2");
    auto entries = parseFsiBlock(&stream);
    ASSERT_EQ(48, entries.size());
    ASSERT_EQ("72.BMP", entries[0].name);
    ASSERT_EQ(0x120c50bc, entries[0].offset);
    ASSERT_EQ(433462, entries[0].size);

    ASSERT_EQ("269.BMP", entries[47].name);
    ASSERT_EQ(0x0959f1ba, entries[47].offset);
    ASSERT_EQ(1018390, entries[47].size);
}

TEST(duden, ParseBFsiBlock) {
    FileStream stream("duden_testfiles/fsiBBlock");
    auto entries = parseFsiBlock(&stream);
    ASSERT_EQ(0, entries.size());
}

TEST(duden, DecodeFixedTreeBofBlock) {
    std::vector<char> decoded;
    std::fstream f("duden_testfiles/bofFixedDeflateBlock");
    f.seekg(0, std::ios_base::end);
    std::vector<char> buf(f.tellg());
    f.seekg(0);
    f.read(&buf[0], buf.size());
    decodeBofBlock(&buf[0], buf.size(), decoded);
    ASSERT_EQ(0x2000, decoded.size());
    ASSERT_EQ(0xa29a3559, crc32(0, (const Bytef*)&decoded[0], decoded.size()));
}

class TestFileSystem : public IFileSystem {
    CaseInsensitiveSet _files;

public:
    TestFileSystem() {
        _files = {"d5snd.idx",
                  "d5snd.fsi",
                  "d5snd.Bof",
                  "DU5NEU.HIC",
                  "du5neu.IDX",
                  "du5neu.bof"};
    }

    std::unique_ptr<dictlsd::IRandomAccessStream> open(fs::path) override {
        return {};
    }

    const CaseInsensitiveSet& files() override {
        return _files;
    }
};

TEST(duden, ParseInfFile) {
    FileStream stream("duden_testfiles/simple.inf");
    auto inf = parseInfFile(&stream);
    ASSERT_EQ(0x400, inf.version);
    ASSERT_EQ(u8"Duden - Das Fremdwörterbuch", inf.name);
    ASSERT_EQ("DU5NEU.HIC", inf.primary.hic);
    ASSERT_EQ("du5neu.idx", inf.primary.idx);
    ASSERT_EQ("du5neu.bof", inf.primary.bof);
    ASSERT_EQ(1, inf.resources.size());
    ASSERT_EQ("d5snd.fsi", inf.resources[0].fsi);
    ASSERT_EQ("d5snd.idx", inf.resources[0].idx);
    ASSERT_EQ("d5snd.bof", inf.resources[0].bof);

    TestFileSystem filesystem;
    fixFileNameCase(inf, &filesystem);
    ASSERT_EQ(u8"Duden - Das Fremdwörterbuch", inf.name);
    ASSERT_EQ("DU5NEU.HIC", inf.primary.hic);
    ASSERT_EQ("du5neu.IDX", inf.primary.idx);
    ASSERT_EQ("du5neu.bof", inf.primary.bof);
    ASSERT_EQ(1, inf.resources.size());
    ASSERT_EQ("d5snd.fsi", inf.resources[0].fsi);
    ASSERT_EQ("d5snd.idx", inf.resources[0].idx);
    ASSERT_EQ("d5snd.Bof", inf.resources[0].bof);

    inf.resources[0].fsi = "";
    fixFileNameCase(inf, &filesystem);
    ASSERT_EQ("", inf.resources[0].fsi);
}

class TestFileSystem2 : public IFileSystem {
    CaseInsensitiveSet _files;

public:
    TestFileSystem2() {
        fs::path root = "/tmp";
        _files = {root / "123_abc.bin", root / "abc.bin", root / "file.ext"};
    }

    std::unique_ptr<dictlsd::IRandomAccessStream> open(fs::path) override {
        return {};
    }

    const CaseInsensitiveSet& files() override {
        return _files;
    }
};

TEST(duden, CaseInsensitiveFileSystemSearch) {
    TestFileSystem2 fileSystem;
    auto found = fileSystem.files().find("/tmp/ABC.bin");
    ASSERT_NE(end(fileSystem.files()), found);
    ASSERT_EQ("/tmp/abc.bin", found->string());
    auto foundExt = findExtension(fileSystem, ".ext");
    ASSERT_EQ("/tmp/file.ext", foundExt.string());
    EXPECT_THROW(findExtension(fileSystem, ".bin"), std::runtime_error);
}

TEST(duden, Unicode) {
    const uint8_t text[] = {
        0x5B, 0x27, 0xA2, 0x74, 0xA2, 0xB0, 0xA2, 0xA5, 0xA2, 0xEE, 0xA2, 0xA8, 0x20,
        0x27, 0xA2, 0xAA, 0xA2, 0xD8, 0xA3, 0x75, 0xA2, 0xB7, 0xA2, 0xEE, 0xA2, 0xBE,
        0x20, 0x27, 0x73, 0x65, 0x6E, 0x74, 0x72, 0x6C, 0x20, 0x27, 0xA2, 0xAE, 0xA3,
        0x2F, 0xA2, 0xDD, 0xA2, 0xB6, 0xA2, 0xDD, 0xA2, 0xB4, 0x2C, 0x00
    };
    auto utf = dudenToUtf8((const char*)text);
    ASSERT_EQ(u8"['ælaɪd 'fɔːsɪz 'sentrl 'jʊərəp,", utf);
}

TEST(duden, UnicodeIgnoreEscapesInsideRefs) {
    const uint8_t text[] = {
        0x4D, 0xA0, 0xFC, 0xA3, 0xE9, 0x6E, 0x64, 0x65, 0x6E, 0x5C, 0x53, 0x7B,
        0x3B, 0x2E, 0x4D, 0x4B, 0x3B, 0x3B, 0x48, 0x61, 0x6E, 0x6E, 0x6F, 0x76,
        0x65, 0x72, 0x73, 0x63, 0x68, 0x20, 0x4D, 0xFC, 0x6E, 0x64, 0x65, 0x6E,
        0x20, 0x7D, 0x4D, 0xA0, 0xFC, 0xA3, 0xE9, 0x6E, 0x64, 0x65, 0x6E, 0x00
    };
    auto utf = dudenToUtf8((const char*)text);
    ASSERT_EQ(u8"Mụ̈nden\\S{;.MK;;Hannoversch Münden }Mụ̈nden", utf);
}

TEST(duden, UnicodeDontIgnoreEscapesInsideTables) {
    const uint8_t text[] = {
        0x4D, 0xA0, 0xFC, 0xA3, 0xE9, 0x6E, 0x64, 0x65, 0x6E,
        0x5C, 0x74, 0x61, 0x62, 0x7B,
        0x4D, 0xA0, 0xFC, 0xA3, 0xE9, 0x6E, 0x64, 0x65, 0x6E,
        0x7D, 0x00
    };
    auto utf = dudenToUtf8((const char*)text);
    ASSERT_EQ(u8"Mụ̈nden\\tab{Mụ̈nden}", utf);
}

TEST(duden, SimpleLdFileTest) {
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    ASSERT_EQ(4, ld.ranges.size());

    ASSERT_EQ("btb", ld.ranges[0].fileName);
    ASSERT_EQ(1, ld.ranges[0].first);
    ASSERT_EQ(122582529, ld.ranges[0].last);

    ASSERT_EQ("btb_pic", ld.ranges[1].fileName);
    ASSERT_EQ(620000000, ld.ranges[1].first);
    ASSERT_EQ(629000000, ld.ranges[1].last);

    ASSERT_EQ("btb_tab", ld.ranges[2].fileName);
    ASSERT_EQ(660000000, ld.ranges[2].first);
    ASSERT_EQ(669000000, ld.ranges[2].last);

    ASSERT_EQ("btb_tim", ld.ranges[3].fileName);
    ASSERT_EQ(670000000, ld.ranges[3].first);
    ASSERT_EQ(671000000, ld.ranges[3].last);

    ASSERT_EQ(10, ld.references.size());

    // builtin
    ASSERT_EQ("WEB", ld.references[0].type);
    ASSERT_EQ("Web", ld.references[0].name);
    ASSERT_EQ("W", ld.references[0].code);

    ASSERT_EQ("PERSON", ld.references[1].type);
    ASSERT_EQ("Person", ld.references[1].name);
    ASSERT_EQ("S", ld.references[1].code);

    ASSERT_EQ("MEDIA", ld.references[6].type);
    ASSERT_EQ("Tabellen", ld.references[6].name);
    ASSERT_EQ("T", ld.references[6].code);
}

TEST(duden, ParseText1) {
    ParsingContext context;
    auto run = parseDudenText(context, "abc \\F{_ADD}def\\F{ADD_}");
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  PlainRun: abc \n"
                    "  AddendumFormattingRun\n"
                    "    PlainRun: def\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ParseText2) {
    auto text = read_all_text("duden_testfiles/article1");
    ParsingContext context;
    auto run = parseDudenText(context, reinterpret_cast<char*>(text.data()));
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  BoldFormattingRun\n"
                    "    PlainRun: Abc\n"
                    "  LineBreakRun\n"
                    "  IdRun: 5000\n"
                    "  LineBreakRun\n"
                    "  PlainRun: (1 \n"
                    "  BoldFormattingRun\n"
                    "    PlainRun: 2\n"
                    "  PlainRun: , 3 \n"
                    "  BoldFormattingRun\n"
                    "    PlainRun: 4\n"
                    "  PlainRun: )\n"
                    "  ReferencePlaceholderRun; code=FWISSEN; num=-1; num2=-1\n"
                    "    TextRun\n"
                    "    PlainRun: 68\n"
                    "  ReferencePlaceholderRun; code=FWISSEN; num=-1; num2=-1\n"
                    "    TextRun\n"
                    "    PlainRun: 60\n"
                    "  SoftLineBreakRun\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ParseText3) {
    auto text = "@1s\\u{i}t f g\\u{o}n@0@8\\\\";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  BoldFormattingRun\n"
                    "    PlainRun: s\n"
                    "    UnderlineFormattingRun\n"
                    "      PlainRun: i\n"
                    "    PlainRun: t f g\n"
                    "    UnderlineFormattingRun\n"
                    "      PlainRun: o\n"
                    "    PlainRun: n\n"
                    "  LineBreakRun\n";
    ASSERT_EQ(expected, tree);

    auto html = printHtml(run);
    ASSERT_EQ("<b>s<u>i</u>t f g<u>o</u>n</b><br>", html);
}

TEST(duden, ParseText4) {
    ParsingContext context;
    auto run = parseDudenText(context, "a\\{b}c");
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  PlainRun: a\\{b}c\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ParseTextReference1) {
    auto text = "\\S{Diskettenformat;:025004230}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  ReferencePlaceholderRun; code=; num=25004230; num2=-1\n"
                    "    TextRun\n"
                    "      PlainRun: Diskettenformat\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ParseTextReference2) {
    auto text = "\\S{Tabelle: table name;.MT:660151833;Tabelle}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  ReferencePlaceholderRun; code=MT; num=660151833; num2=-1\n"
                    "    TextRun\n"
                    "      PlainRun: Tabelle: table name\n"
                    "    PlainRun: Tabelle\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ParseTextReference3) {
    std::string text = "\\S{@7A@0bc@7d@0ef;:013710058}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  ReferencePlaceholderRun; code=; num=13710058; num2=-1\n"
                    "    TextRun\n"
                    "      BoldFormattingRun\n"
                    "        PlainRun: A\n"
                    "      PlainRun: bc\n"
                    "      BoldFormattingRun\n"
                    "        PlainRun: d\n"
                    "      PlainRun: ef\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ParseTextReferenceWithEscapedSemicolon) {
    auto text = "\\S{abc@; ;:100199516}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  ReferencePlaceholderRun; code=; num=100199516; num2=-1\n"
                    "    TextRun\n"
                    "      PlainRun: abc; \n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ResolveArticleReference) {
    auto text = "\\S{Diskettenformat;:025004230}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  ArticleReferenceRun; offset=25004230\n"
                    "    TextRun\n"
                    "      PlainRun: Diskettenformat\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ResolveTableReference) {
    auto text = "\\S{Tabelle: table name;.MT:660151833;Tabelle}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  TableReferenceRun; offset=151833; file=btb_tab\n"
                    "    TextRun\n"
                    "      PlainRun: Tabelle: table name\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ResolvePictureReference) {
    auto text = "\\S{;.MBB:620127616;Astana}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  PictureReferenceRun; offset=127616; file=btb_pic; cr=; image=\n"
                    "    PlainRun: Astana\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ResolveWebReference) {
    auto text = "\\S{http://www.example.com/;.MW:004549700;www.example.de/;http://www.example.de/}abc";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  WebReferenceRun; link=http://www.example.de/\n"
                    "    TextRun\n"
                    "      PlainRun: http://www.example.com/\n"
                    "  PlainRun: abc\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ResolveInlineImageReference) {
    auto text = "\\S{;.Ieuro.bmp;T}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  InlineImageRun; name=euro.bmp; secondary=\n";
    ASSERT_EQ(expected, tree);
    ASSERT_EQ(u8"[s]euro.bmp[/s]", printDsl(run));
}

TEST(duden, ResolveAudioReference) {
    auto text = u8"\\S{;.Ispeaker.bmp;T;à la longue.Adp}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  InlineImageRun; name=speaker.bmp; secondary=à la longue.wav\n";
    ASSERT_EQ(expected, tree);
    ASSERT_EQ(u8"[s]à la longue.wav[/s]", printDsl(run));
}

class TestFileSystem3 : public IFileSystem {
    CaseInsensitiveSet _files;

public:
    TestFileSystem3() {
        fs::path root = "";
        _files = {u8"123.bmp",
                  u8"euro.bmp",
                  u8"АбfD.BMP",
                  u8"UNABKöMMLICH1V.WAV"};
    }
     std::unique_ptr<dictlsd::IRandomAccessStream> open(fs::path) override {
        return {};
    }

    const CaseInsensitiveSet& files() override {
        return _files;
    }
};

TEST(duden, ResolveInlineImageReferenceInconstistentCase) {
    auto text = u8"\\S{;.IаБFd.bmp;T}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    TestFileSystem3 fs;
    resolveReferences(context, run, ld, &fs);
    ASSERT_EQ(u8"[s]АбfD.BMP[/s]", printDsl(run));
}

TEST(duden, ResolveInlineImageReferenceInconstistentCase2) {
    auto text = u8"\\S{;.Ispeaker.bmp;T;unabkömmlich1v.adp}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    TestFileSystem3 fs;
    resolveReferences(context, run, ld, &fs);
    ASSERT_EQ(u8"[s]UNABKöMMLICH1V.WAV[/s]", printDsl(run));
}

TEST(duden, InlinePictureReference) {
    auto text = "\\S{;.MBB:620000166;Caption}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);

    ResourceFiles files;
    files["btb_pic"] = std::make_unique<std::ifstream>("duden_testfiles/duden_encoded_pic");
    inlineReferences(context, run, files);

    auto picture = dynamic_cast<PictureReferenceRun*>(run->runs().front());
    ASSERT_NE(nullptr, picture);
    auto description = dynamic_cast<PlainRun*>(picture->description()->runs().front());
    ASSERT_NE(nullptr, description);

    ASSERT_EQ(u8"© Bibliographisches Institut & F. A. Brockhaus, Mannheim", picture->copyright());
    ASSERT_EQ(u8"b5bi1607.BMP", picture->imageFileName());
    ASSERT_EQ(u8"Die vom Fahrstrahl in gleichen Zeitintervallen überstrichenen Flächen (rosa) sind gleich groß", description->text());
}

TEST(duden, ParseTextTable1) {
    auto text = read_all_text("duden_testfiles/table1");
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                     "  TableRun\n"
                     "    TableTag 1; from=2; to=-1\n"
                     "    TableTag 2; from=3; to=-1\n"
                     "    TableTag 3; from=-1; to=-1\n"
                     "    SoftLineBreakRun\n"
                     "    TableTag 4; from=-1; to=-1\n"
                     "    TableTag 5; from=1; to=3\n"
                     "    TableTag 6; from=1; to=3\n"
                     "    SoftLineBreakRun\n"
                     "    TableTag 4; from=-1; to=-1\n"
                     "    TableTag 5; from=1; to=3\n"
                     "    TableTag 6; from=1; to=3\n"
                     "    SoftLineBreakRun\n"
                     "    TableTag 4; from=-1; to=-1\n"
                     "    TableTag 5; from=1; to=3\n"
                     "    TableTag 6; from=1; to=3\n"
                     "    SoftLineBreakRun\n"
                     "    TableTag 8; from=-1; to=-1\n"
                     "    TableTag 9; from=1; to=2\n"
                     "    TableTag 10; from=1; to=2\n"
                     "    SoftLineBreakRun\n"
                     "    TableTag 8; from=-1; to=-1\n"
                     "    TableTag 9; from=1; to=2\n"
                     "    TableTag 10; from=1; to=2\n"
                     "    SoftLineBreakRun\n"
                     "    TableTag 8; from=-1; to=-1\n"
                     "    TableTag 9; from=1; to=2\n"
                     "    TableTag 10; from=1; to=2\n"
                     "    SoftLineBreakRun\n"
                     "    TableTag 8; from=-1; to=-1\n"
                     "    TableTag 9; from=1; to=2\n"
                     "    TableTag 10; from=1; to=2\n"
                     "    SoftLineBreakRun\n"
                     "    TableCellRun\n"
                     "      BoldFormattingRun\n"
                     "        PlainRun: bold cell\n"
                     "        SoftLineBreakRun\n"
                     "    TableCellRun\n"
                     "      PlainRun: plain cell\n"
                     "      SoftLineBreakRun\n"
                     "    TableCellRun\n"
                     "      BoldFormattingRun\n"
                     "        ItalicFormattingRun\n"
                     "          PlainRun: bold italic cell\n"
                     "          SoftLineBreakRun\n"
                     "    TableCellRun\n"
                     "      PlainRun: random braces {\n"
                     "      SoftLineBreakRun\n"
                     "    TableCellRun\n"
                     "      PlainRun: sub\n"
                     "      SubscriptFormattingRun\n"
                     "        PlainRun: scripted\n"
                     "      PlainRun: cell\n"
                     "      SoftLineBreakRun\n"
                     "    TableCellRun\n"
                     "      ItalicFormattingRun\n"
                     "        PlainRun: italic cell\n"
                     "        SoftLineBreakRun\n"
                     "  SoftLineBreakRun\n";
    ASSERT_EQ(expected, tree);

    auto table = dynamic_cast<TableRun*>(run->runs().front());
    auto cell = printTree(table->table()->cell(1, 2));
    expected = "    TableCellRun\n"
               "      PlainRun: sub\n"
               "      SubscriptFormattingRun\n"
               "        PlainRun: scripted\n"
               "      PlainRun: cell\n"
               "      SoftLineBreakRun\n";
    ASSERT_EQ(expected, cell);
}

TEST(duden, ParseTextTableWithExtraCells) {
    auto text = read_all_text("duden_testfiles/table_several_extra_cells_and_footer");
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  TableRun\n"
                    "    TableTag 1; from=1; to=-1\n"
                    "    TableTag 2; from=1; to=-1\n"
                    "    TableTag 3; from=-1; to=-1\n"
                    "    SoftLineBreakRun\n"
                    "    TableCellRun\n"
                    "      PlainRun: content\n"
                    "      SoftLineBreakRun\n"
                    "      SoftLineBreakRun\n"
                    "      SoftLineBreakRun\n"
                    "      SoftLineBreakRun\n"
                    "  SoftLineBreakRun\n"
                    "  ReferencePlaceholderRun; code=; num=12345; num2=-1\n"
                    "    TextRun\n"
                    "    PlainRun: abcde\n"
                    "  LineBreakRun\n"
                    "  SoftLineBreakRun\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ParseTextTableWithCurlyBracesInCells) {
    auto text = read_all_text("duden_testfiles/table_curly_braces");
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  TableRun\n"
                    "    TableTag 1; from=2; to=-1\n"
                    "    TableTag 2; from=2; to=-1\n"
                    "    TableTag 3; from=-1; to=-1\n"
                    "    SoftLineBreakRun\n"
                    "    TableCellRun\n"
                    "      PlainRun: 1{2\n"
                    "      SoftLineBreakRun\n"
                    "    TableCellRun\n"
                    "      PlainRun: 1}2\n"
                    "      SoftLineBreakRun\n"
                    "    TableCellRun\n"
                    "      PlainRun: 1{2}\n"
                    "      SoftLineBreakRun\n"
                    "    TableCellRun\n"
                    "      PlainRun: }1{2\n"
                    "      SoftLineBreakRun\n"
                    "      SoftLineBreakRun\n"
                    "  SoftLineBreakRun\n";
    ASSERT_EQ(expected, tree);
}

/*
 ____________________
|_0,0__|_0,1_________|
|_1,0_________| 1,2  |
|_2,0__|_2,1__|______|

*/
TEST(duden, ParseTextTableWithMergedCells) {
    auto text = read_all_text("duden_testfiles/table_merged_cells");
    ParsingContext context;
    auto run = parseDudenText(context, text);

    auto table = dynamic_cast<TableRun*>(run->runs().front());

    auto assertCell = [&](int r, int c, auto text, int hspan, int vspan) {
        auto actualText = printDsl(table->table()->cell(r, c));
        boost::algorithm::trim(actualText);
        ASSERT_EQ(text, actualText);
        ASSERT_EQ(hspan, table->table()->hspan(r, c));
        ASSERT_EQ(vspan, table->table()->vspan(r, c));
        ASSERT_EQ(false, table->table()->merged(r, c));
    };

    assertCell(0, 0, "0,0", 1, 1);
    assertCell(0, 1, "0,1", 2, 1);
    assertCell(1, 0, "1,0", 2, 1);
    assertCell(1, 2, "1,2", 1, 2);
    assertCell(2, 0, "2,0", 1, 1);
    assertCell(2, 1, "2,1", 1, 1);

    ASSERT_EQ(true, table->table()->merged(0, 2));
    ASSERT_EQ(true, table->table()->merged(1, 1));
    ASSERT_EQ(true, table->table()->merged(2, 2));
}

TEST(duden, InlineTableReference) {
    auto text = "\\S{Tabelle: table name;.MT:660000000;Tabelle}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);

    ResourceFiles files;
    files["btb_tab"] = std::make_unique<std::ifstream>("duden_testfiles/tab_file");
    inlineReferences(context, run, files);

    auto tableRef = dynamic_cast<TableReferenceRun*>(run->runs().front());
    ASSERT_NE(nullptr, tableRef);
    ASSERT_EQ("Table: MT", tableRef->mt());
    ASSERT_EQ("Tabelle: table name", printDsl(tableRef->referenceCaption()));

    auto expected = "TextRun\n"
                    "  BoldFormattingRun\n"
                    "    PlainRun: Table: Name\n"
                    "  IdRun: 12345\n"
                    "  SoftLineBreakRun\n"
                    "  TableRun\n"
                    "    TableTag 1; from=2; to=-1\n"
                    "    TableTag 2; from=3; to=-1\n"
                    "    TableTag 3; from=-1; to=-1\n"
                    "    SoftLineBreakRun\n"
                    "    TableCellRun\n"
                    "      BoldFormattingRun\n"
                    "        PlainRun: bold cell\n"
                    "        SoftLineBreakRun\n"
                    "    TableCellRun\n"
                    "      PlainRun: plain cell\n"
                    "      SoftLineBreakRun\n"
                    "    TableCellRun\n"
                    "      BoldFormattingRun\n"
                    "        ItalicFormattingRun\n"
                    "          PlainRun: bold italic cell\n"
                    "          SoftLineBreakRun\n"
                    "    TableCellRun\n"
                    "      PlainRun: random braces {\n"
                    "      SoftLineBreakRun\n"
                    "    TableCellRun\n"
                    "      PlainRun: sub\n"
                    "      SubscriptFormattingRun\n"
                    "        PlainRun: scripted\n"
                    "      PlainRun: cell\n"
                    "      SoftLineBreakRun\n"
                    "    TableCellRun\n"
                    "      ItalicFormattingRun\n"
                    "        PlainRun: italic cell\n"
                    "        SoftLineBreakRun\n"
                    "  SoftLineBreakRun\n"
                    "  PlainRun: Footer 1\n"
                    "  LineBreakRun\n"
                    "  SoftLineBreakRun\n"
                    "  PlainRun: Footer 2\n"
                    "  LineBreakRun\n"
                    "  SoftLineBreakRun\n"
                    "  ReferencePlaceholderRun; code=; num=0; num2=-1\n"
                    "    TextRun\n"
                    "    PlainRun: article\n"
                    "  LineBreakRun\n"
                    "  SoftLineBreakRun\n";
    ASSERT_EQ(expected, printTree(tableRef->content()));
}

TEST(duden, PrintHtmlSupSub) {
    std::string text = "a\\sup{b}c\\sub{d}e";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  PlainRun: a\n"
                    "  SuperscriptFormattingRun\n"
                    "    PlainRun: b\n"
                    "  PlainRun: c\n"
                    "  SubscriptFormattingRun\n"
                    "    PlainRun: d\n"
                    "  PlainRun: e\n";
    ASSERT_EQ(expected, printTree(run));
    auto tree = printHtml(run);
    ASSERT_EQ("a<sup>b</sup>c<sub>d</sub>e", tree);
}

TEST(duden, ParseColors) {
    std::string text = "\\F{_00 00 FF}abc\\F{00 00 FF_}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  ColorFormattingRun; rgb=0000ff; name=blue\n"
                    "    PlainRun: abc\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, ParseColorsWithoutSpaces) {
    std::string text = "\\F{_FF0000}abc\\F{FF0000_}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  ColorFormattingRun; rgb=ff0000; name=red\n"
                    "    PlainRun: abc\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, SimpleFormatting) {
    std::string text = "\\F{_FF 00 00}color\\F{FF 00 00_}"
                       "@1bold"
                       "@3bold italic"
                       "@2italic@0"
                       "@1bold \\u{underline}@0"
                       "plain \\F{_ADD}a\\u{d}d\\F{ADD_}"
                       "plain \\F{_UE}a\\u{d}d\\F{UE_}"
                       "plain a\\sub{sub} and a\\sup{sup}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "[c red]color[/c]"
                    "[b]bold[/b]"
                    "[b][i]bold italic[/i][/b]"
                    "[i]italic[/i]"
                    "[b]bold [u]underline[/u][/b]"
                    "plain (a[u]d[/u]d)"
                    "plain (a[u]d[/u]d)"
                    "plain a[sub]sub[/sub] and a[sup]sup[/sup]";
    ASSERT_EQ(expected, printDsl(run));
    expected = "<font color=\"red\">color</font>"
               "<b>bold</b>"
               "<b><i>bold italic</i></b>"
               "<i>italic</i>"
               "<b>bold <u>underline</u></b>"
               "plain (a<u>d</u>d)"
               "plain (a<u>d</u>d)"
               "plain a<sub>sub</sub> and a<sup>sup</sup>";
    ASSERT_EQ(expected, printHtml(run));
}

TEST(duden, ParseTab) {
    std::string text = "\\eb{0}\\eb{1}\\ee";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  TabRun: 0\n"
                    "  TabRun: 1\n"
                    "  TabRun: -1\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, ParseWebLink) {
    std::string text = "\\F{_WebLink}abc\\F{WebLink_}de";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  WebLinkFormattingRun\n"
                    "    PlainRun: abc\n"
                    "  PlainRun: de\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, ParseEscapedBackslash) {
    std::string text = "@\\@\\a@\\b@\\c";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  PlainRun: \\\\a\\b\\c\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, ParseUpwardsArrow) {
    std::string text = "@Sabc";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    ASSERT_EQ(u8"↑abc", printDsl(run));
}

TEST(duden, ParseNestedStickyTag) {
    std::string text = "1\\sup{@2n@0}@22\\sub{3}4";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  PlainRun: 1\n"
                    "  SuperscriptFormattingRun\n"
                    "    ItalicFormattingRun\n"
                    "      PlainRun: n\n"
                    "  ItalicFormattingRun\n"
                    "    PlainRun: 2\n"
                    "    SubscriptFormattingRun\n"
                    "      PlainRun: 3\n"
                    "    PlainRun: 4\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, ParseId) {
    std::string text = "a@C%ID=12345\nb";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  PlainRun: a\n"
                    "  IdRun: 12345\n"
                    "  SoftLineBreakRun\n"
                    "  PlainRun: b\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, ParseEmptyId) {
    std::string text = "a@C%ID=\nb";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  PlainRun: a\n"
                    "  IdRun: -1\n"
                    "  SoftLineBreakRun\n"
                    "  PlainRun: b\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, ParseSoftLineBreak) {
    std::string text = "a\nb\r\nc";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  PlainRun: a\n"
                    "  SoftLineBreakRun\n"
                    "  PlainRun: b\n"
                    "  SoftLineBreakRun\n"
                    "  PlainRun: c\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, IgnoreUnknownReferencesInDsl) {
    std::string text = "\\S{;.FWISSEN;30}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    ASSERT_EQ("", printDsl(run));
}

TEST(duden, HandleNewLinesInDsl) {
    ParsingContext context;
    auto run = parseDudenText(context, "a\\\\b");
    ASSERT_EQ("a\nb", printDsl(run));
    run = parseDudenText(context, "a\nb");
    ASSERT_EQ("ab", printDsl(run));
}

TEST(duden, HandleAddendumInDsl) {
    ParsingContext context;
    auto run = parseDudenText(context, "Abc \\F{_ADD}123\\F{ADD_}");
    ASSERT_EQ("Abc (123)", printDsl(run));
}

TEST(duden, HandleRightAlignmentInDsl) {
    ParsingContext context;
    auto run = parseDudenText(context, "\\F{_Right}abc\\F{Right_}");
    ASSERT_EQ("abc", printDsl(run));
}

TEST(duden, EscapeSquareBracketsInDsl) {
    ParsingContext context;
    auto run = parseDudenText(context, "[abc]");
    ASSERT_EQ("\\[abc\\]", printDsl(run));
}

TEST(duden, PrintInlineRenderedTable) {
    auto text = read_all_text("duden_testfiles/table1");
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto tableRun = dynamic_cast<TableRun*>(run->runs().front());
    ASSERT_NE(nullptr, tableRun);
    tableRun->setRenderedName("table.bmp");
    ASSERT_EQ("[s]table.bmp[/s]", printDsl(run));
}

TEST(duden, InlineRenderAndPrintPicture) {
    auto text = "\\S{;.MBB:620000166;Caption}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    ResourceFiles files;
    files["btb_pic"] = std::make_unique<std::ifstream>("duden_testfiles/duden_encoded_pic");
    inlineReferences(context, run, files);
    std::string name;
    TableRenderer renderer([&](auto, auto n) { name = n; }, [](auto){return std::vector<char>();});
    renderer.render(run);
    auto expected = "----------\n"
                    "[b]2. keplersches Gesetz:[/b]\n"
                    "Die vom Fahrstrahl in gleichen Zeitintervallen überstrichenen Flächen (rosa) sind gleich groß\n"
                    "[s]b5bi1607.BMP[/s]\n"
                    "© Bibliographisches Institut & F. A. Brockhaus, Mannheim\n"
                    "----------\n";
    ASSERT_EQ(expected, printDsl(run));
}

TEST(duden, DedupHeading) {
    ParsingContext context;
    auto heading = parseDudenText(context, "Heading");
    auto article = parseDudenText(context, "Heading\nAnd then body");
    auto res = dedupHeading(heading, article);
    ASSERT_EQ(true, res);
    ASSERT_EQ("And then body", printDsl(article));

    article = parseDudenText(context, "@1Heading@0\nAnd then body");
    res = dedupHeading(heading, article);
    ASSERT_EQ(true, res);
    ASSERT_EQ("And then body", printDsl(article));

    article = parseDudenText(context, "    \n@1Heading@0\nAnd then body");
    res = dedupHeading(heading, article);
    ASSERT_EQ(true, res);
    ASSERT_EQ("And then body", printDsl(article));

    article = parseDudenText(context, "H e a d i n g");
    res = dedupHeading(heading, article);
    ASSERT_EQ(false, res);

    article = parseDudenText(context, "Hea\nding");
    res = dedupHeading(heading, article);
    ASSERT_EQ(false, res);
}

TEST(duden, ParseRangeReference) {
    ParsingContext context;
    auto run = parseDudenText(context,
                                  "@1Vientiane@0\\S{;.G1:000035-000035;@7Vientiane@0}@8\\\\\n"
                                  "@9@C%ID=51049700\n"
                                  "@8\\\\\n"
                                  "@9\n"
                                  "[vjɛnti'aːnə]");
    auto expected = "TextRun\n"
                    "  BoldFormattingRun\n"
                    "    PlainRun: Vientiane\n"
                    "  ReferencePlaceholderRun; code=G1; num=35; num2=35\n"
                    "    TextRun\n"
                    "    BoldFormattingRun\n"
                    "      PlainRun: Vientiane\n"
                    "  LineBreakRun\n"
                    "  SoftLineBreakRun\n"
                    "  IdRun: 51049700\n"
                    "  SoftLineBreakRun\n"
                    "  LineBreakRun\n"
                    "  SoftLineBreakRun\n"
                    "  SoftLineBreakRun\n"
                    "  PlainRun: [vjɛnti'aːnə]\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, ParseEscapedApostrophe) {
    ParsingContext context;
    auto run = parseDudenText(context, "a\\'b");
    ASSERT_EQ("a'b", printDsl(run));
}

TEST(duden, ParseEscapedAtSign) {
    ParsingContext context;
    auto run = parseDudenText(context, "a@@b");
    ASSERT_EQ("a@b", printDsl(run));
}

TEST(duden, ParsePersonTag) {
    ParsingContext context;
    auto run = parseDudenText(context, "@C%NA=\"abc\"\n");
    ASSERT_EQ("", printDsl(run));
}

TEST(duden, ParseCbmm) {
    ParsingContext context;
    auto run = parseDudenText(context, "a@CBMM2004 - abc\nb");
    ASSERT_EQ("ab", printDsl(run));
}

TEST(duden, IgnoreUnknownEscapes) {
    ParsingContext context;
    auto run = parseDudenText(context, "a@bc");
    ASSERT_EQ("abc", printDsl(run));
}

TEST_F(duden_qt, InlineRenderAndPrintTable) {
    auto text = "\\S{Tabelle: table name;.MT:660000000;Tabelle}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    ResourceFiles files;
    files["btb_tab"] = std::make_unique<std::ifstream>("duden_testfiles/tab_file");
    inlineReferences(context, run, files);
    std::string name;
    TableRenderer renderer([&](auto, auto n) { name = n; }, [](auto){return std::vector<char>();});
    renderer.render(run);
    auto expected = bformat(
                "----------\n"
                "Tabelle: table name\n"
                "[b]Table: Name[/b]"
                "[s]%s[/s]"
                "Footer 1\n"
                "Footer 2\n"
                "----------\n", name);
    ASSERT_EQ(expected, printDsl(run));
}

TEST(duden, IgnoreMismatchedTokensInIllformedText) {
    std::string text = "a\\F{_WebLink}b\\F{WebLink_}c\\F{WebLink_}d";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  PlainRun: a\n"
                    "  WebLinkFormattingRun\n"
                    "    PlainRun: b\n"
                    "  PlainRun: c\n"
                    "  PlainRun: d\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, IgnoreIllformedTableReferences) {
    // even Duden can't parse this
    std::string text = "\\S{here comes the semicolon; and text continues;.MT:660042728;Tabelle}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  ReferencePlaceholderRun; code=; num=-1; num2=-1\n"
                    "    TextRun\n"
                    "      PlainRun: here comes the semicolon\n"
                    "    PlainRun: ; and text continues\n"
                    "    PlainRun: .MT:660042728\n"
                    "    PlainRun: Tabelle\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, HandleEmbeddedImagesInHtml) {
    auto text = "\\S{;.Ieuro.eXt;T}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    resolveReferences(context, run, {}, nullptr);
    std::string requestedName;
    auto html = printHtml(run, [&](auto name) {
        requestedName = name;
        std::vector<char> vec;
        vec.push_back('1');
        vec.push_back('2');
        vec.push_back('3');
        vec.push_back('4');
        return vec;
    });
    ASSERT_EQ("euro.eXt", requestedName);
    auto expected = "<img src=\"data:image/ext;base64,MTIzNA==\">";
    ASSERT_EQ(expected, html);
}

TEST(duden, HandleEmbeddedImagesInHtml2) {
    auto text = "\\S{Tabelle: table name;.MT:660000000;Tabelle}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream("duden_testfiles/simple.ld");
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    ResourceFiles files;
    files["btb_tab"] = std::make_unique<std::ifstream>("duden_testfiles/tab_file_embedded_image");
    inlineReferences(context, run, files);
    resolveReferences(context, run, ld, nullptr); // resolve inlined references

    std::string requestedName;
    auto html = printHtml(run, [&](auto name) {
        requestedName = name;
        std::vector<char> vec;
        vec.push_back('1');
        vec.push_back('2');
        vec.push_back('3');
        vec.push_back('4');
        return vec;
    });
    ASSERT_EQ("euro.bmp", requestedName);
    auto expected = "<img src=\"data:image/bmp;base64,MTIzNA==\">";
    ASSERT_EQ(true, html.find(expected) != std::string::npos);
}

TEST(duden, InlineArticleReference) {
    ParsingContext context;
    auto run = parseDudenText(context, "\\S{ArticleName;:000620083}\\S{SameName;:000620084}");
    resolveReferences(context, run, {}, nullptr);
    resolveArticleReferences(run, [](auto offset) {
        if (offset == 620083)
            return "ResolvedName";
        if (offset == 620084)
            return "SameName";
        return "error";
    });
    ASSERT_EQ("[ref]ResolvedName[/ref] (ArticleName)[ref]SameName[/ref]", printDsl(run));
}

TEST(duden, GroupHicEntries1) {
    HicLeaf a { "a \\F{_UE}123\\F{UE_} $$$$  -1 101 -16", HicEntryType::VariantWith, -1u };
    HicLeaf b { "b $$$$  -1 101 -16", HicEntryType::Reference, -1u };
    HicLeaf c { "c $$$$  -1 101 -16", HicEntryType::VariantWith, -1u };
    auto groups = groupHicEntries({a,c,b});
    ASSERT_EQ(1, groups.size());
    ASSERT_EQ((std::vector{"a \\F{_UE}123\\F{UE_}"s, "b"s, "c"s}), groups[100].headings);
}

TEST(duden, GroupHicEntries2) {
    HicLeaf a { "a", HicEntryType::Plain, 100 };
    auto groups = groupHicEntries({a});
    ASSERT_EQ(1, groups.size());
    ASSERT_EQ((std::vector{"a"s}), groups[100].headings);
}

TEST(duden, GroupHicEntries3) {
    HicLeaf a { "[1]a[v]", HicEntryType::Variant, 100 };
    HicLeaf b { "1av $$$$  -1 101 -16", HicEntryType::VariantWith, -100u };
    HicLeaf c { "a $$$$  -1 101 -16", HicEntryType::VariantWithout, -100u };
    auto groups = groupHicEntries({a,c,b});
    ASSERT_EQ(1, groups.size());
    ASSERT_EQ((std::vector{"1av"s, "a"s}), groups[100].headings);
}

TEST(duden, GroupHicEntries4) {
    HicLeaf a { "a[v]", HicEntryType::Variant, 100 };
    HicLeaf b { "av $$$$  -1 101 -16", HicEntryType::VariantWith, -1u };
    HicLeaf c { "z $$$$  -1 101 -16", HicEntryType::VariantWithout, -1u };
    auto groups = groupHicEntries({b,c,a});
    ASSERT_EQ(1, groups.size());
    ASSERT_EQ((std::vector{"av"s, "z"s}), groups[100].headings);
}

TEST(duden, GroupHicEntries5) {
    HicLeaf a { "a $$$$  -1 101 -16", HicEntryType::Reference, -1u };
    HicLeaf b { "b $$$$  -1 101 -16", HicEntryType::Reference, -1u };
    HicLeaf c { "c \\F{_ADD}add\\F{ADD_}", HicEntryType::Plain, 200u };
    HicLeaf d { "d", HicEntryType::Plain, 300 };
    auto groups = groupHicEntries({d,c,b,a});
    ASSERT_EQ(3, groups.size());
    ASSERT_EQ((std::vector{"a"s, "b"s}), groups[100].headings);
    ASSERT_EQ((std::vector{"c \\F{_ADD}add\\F{ADD_}"s}), groups[200].headings);
    ASSERT_EQ((std::vector{"d"s}), groups[300].headings);
}

TEST(duden, GroupHicEntries6) {
    HicLeaf a { "a", HicEntryType::Plain, 10 };
    HicLeaf b { "b", HicEntryType::Plain, 20 };
    HicLeaf c { "c $$$$  -1 21 -16", HicEntryType::Reference, 1234 };
    HicLeaf d { "d $$$$  -1 11 -16", HicEntryType::Reference, 1234 };
    HicLeaf e { "e $$$$  -1 11 -16", HicEntryType::Reference, 1234 };
    HicLeaf f { "f $$$$  -1 26 -16", HicEntryType::VariantWith, 1234 };
    HicLeaf g { "g", HicEntryType::Variant, 25 };
    HicLeaf h { "h $$$$  -1 26 -16", HicEntryType::VariantWithout, 1234 };
    auto groups = groupHicEntries({a,b,c,d,e,f,g,h});
    ASSERT_EQ(3, groups.size());
    ASSERT_EQ((std::vector{"a"s, "d"s, "e"s}), groups[10].headings);
    ASSERT_EQ((std::vector{"b"s, "c"s}), groups[20].headings);
    ASSERT_EQ((std::vector{"f"s, "h"s}), groups[25].headings);
    ASSERT_EQ(10, groups[10].articleSize);
    ASSERT_EQ(5, groups[20].articleSize);
    ASSERT_EQ(-1, groups[25].articleSize);
}

TEST(duden, HandleNewLinesInHtml) {
    auto text = "a\\\\b\nc";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto html = printHtml(run);
    ASSERT_EQ("a<br>bc", html);
}
