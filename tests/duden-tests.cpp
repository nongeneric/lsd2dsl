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
#include "lib/duden/Writer.h"
#include "lib/duden/HtmlRenderer.h"
#include "lib/duden/TableRenderer.h"
#include "lib/common/bformat.h"
#include "test-utils.h"
#include <QApplication>
#include "zlib.h"
#include <boost/algorithm/string.hpp>
#include <map>
#include <qt5/QtWidgets/qapplication.h>

using namespace duden;
using namespace dictlsd;
using namespace std::literals;

class duden_qt : public ::testing::Test {
    int _c = 1;
    char _appName[8] = "testapp";
    char* _args[1]{_appName};
    QApplication _app;

public:
    duden_qt() : _app(_c, _args) { }
    ~duden_qt() = default;
};

TEST(duden, HicNodeTest1) {
    FileStream stream(testPath("duden_testfiles/HicNode99"));

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
    FileStream stream(testPath("duden_testfiles/HicNode10c5"));

    auto block = parseHicNode45(&stream);
    auto at = [&](int i) { return std::get<HicLeaf>(block[i]); };

    ASSERT_EQ(20, block.size());
    ASSERT_EQ(u8"absent", at(1).heading);
    ASSERT_EQ(u8"absentia", at(2).heading);
    ASSERT_EQ(u8"absentieren", at(3).heading);
    ASSERT_EQ(u8"Absolventin", at(15).heading);
}

TEST(duden, HicNodeTest3) {
    FileStream stream(testPath("duden_testfiles/block_hic_v6"));

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
    FileStream stream(testPath("duden_testfiles/block_2847_heading_encoding"));

    auto block = parseHicNode6(&stream);
    auto at = [&](int i) { return std::get<HicLeaf>(block[i]); };

    ASSERT_EQ(18, block.size());
    ASSERT_EQ(u8"8-bit-Zeichensatz", at(0).heading);
    ASSERT_EQ(u8"a.", at(10).heading);
    ASSERT_EQ(u8"à", at(11).heading);
}

TEST(duden, HicNodeTest_ParseNonLeafs) {
    FileStream stream(testPath("duden_testfiles/HicNode6a"));

    auto block = parseHicNode45(&stream);
    auto at = [&](int i) { return std::get<HicNode>(block[i]); };

    ASSERT_EQ(2, block.size());
    EXPECT_EQ(u8"Zusätze", at(0).heading);
    EXPECT_EQ(13, at(0).count);
    EXPECT_EQ(-67283, at(0).delta);
    EXPECT_EQ(151, at(0).hicOffset);

    EXPECT_EQ(u8"Wörterverzeichnis", at(1).heading);
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
    FileStream stream(testPath("duden_testfiles/fsiSingleItemBlock"));

    auto entries = parseFsiBlock(&stream);

    ASSERT_EQ(1, entries.size());
    ASSERT_EQ("EURO.BMP", entries[0].name);
    ASSERT_EQ(0, entries[0].offset);
    ASSERT_EQ(1318, entries[0].size);
}

TEST(duden, ParseSingleItemFsiBlockUnicode) {
    FileStream stream(testPath("duden_testfiles/fsiSingleItemBlockUnicode"));

    auto entries = parseFsiBlock(&stream);

    ASSERT_EQ(1, entries.size());
    ASSERT_EQ(u8"EURä.BMP", entries[0].name);
    ASSERT_EQ(0, entries[0].offset);
    ASSERT_EQ(1318, entries[0].size);
}

TEST(duden, ParseCFsiBlock) {
    FileStream stream(testPath("duden_testfiles/fsiCBlock"));
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
    FileStream stream(testPath("duden_testfiles/fsiCBlock2"));
    auto entries = parseFsiBlock(&stream);
    ASSERT_EQ(48, entries.size());
    ASSERT_EQ("72.BMP", entries[0].name);
    ASSERT_EQ(0x120c50bc, entries[0].offset);
    ASSERT_EQ(433462, entries[0].size);

    ASSERT_EQ("269.BMP", entries[47].name);
    ASSERT_EQ(0x0959f1ba, entries[47].offset);
    ASSERT_EQ(1018390, entries[47].size);
}

TEST(duden, ParseCFsiBlock3) {
    FileStream stream(testPath("duden_testfiles/fsiCBlock3"));
    auto entries = parseFsiBlock(&stream);
    ASSERT_EQ(39, entries.size());
    EXPECT_EQ("BMM00434.BMP", entries[0].name);
    EXPECT_EQ(0x8e6050, entries[0].offset);
    EXPECT_EQ(116278, entries[0].size);

    EXPECT_EQ("BMM00650.BMP", entries[38].name);
    EXPECT_EQ(0x6ed12a, entries[38].offset);
    EXPECT_EQ(123772, entries[38].size);
}

TEST(duden, ParseCFsiBlock4) {
    FileStream stream(testPath("duden_testfiles/fsiCBlock4"));
    auto entries = parseFsiBlock(&stream);
    ASSERT_EQ(40, entries.size());
    EXPECT_EQ("B5BI1363.BMP", entries[0].name);
    EXPECT_EQ(1170336, entries[0].offset);
    EXPECT_EQ(29346, entries[0].size);

    EXPECT_EQ("BMM00423.BMP", entries[39].name);
    EXPECT_EQ(0, entries[39].offset);
    EXPECT_EQ(108278, entries[39].size);
}

TEST(duden, ParseBFsiBlock) {
    FileStream stream(testPath("duden_testfiles/fsiBBlock"));
    auto entries = parseFsiBlock(&stream);
    ASSERT_EQ(0, entries.size());
}

TEST(duden, DecodeFixedTreeBofBlock) {
    std::vector<char> decoded;
    std::ifstream f(testPath("duden_testfiles/bofFixedDeflateBlock"));
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
    std::vector<std::unique_ptr<std::string>> _lds;

public:
    TestFileSystem(bool empty = false) {
        if (empty)
            return;
        _files = {"d5snd.idx",
                  "d5snd.fsi",
                  "d5snd.Bof",
                  "DU5NEU.HIC",
                  "du5neu.IDX",
                  "du5neu.bof",
                  "du5neU.Ld"};
    }

    std::unique_ptr<dictlsd::IRandomAccessStream> open(std::filesystem::path path) override {
        if (boost::algorithm::to_lower_copy(path.extension().u8string()) == ".ld") {
            _lds.push_back(std::make_unique<std::string>());
            auto& ld = _lds.back();
            *ld = "K" + path.stem().u8string();
            return std::make_unique<dictlsd::InMemoryStream>(ld->c_str(), ld->size());
        }
        throw std::runtime_error("");
    }

    CaseInsensitiveSet& files() override {
        return _files;
    }
};

TEST(duden, ParseInfFile) {
    FileStream stream(testPath("duden_testfiles/simple.inf"));
    TestFileSystem filesystem;
    auto inf = parseInfFile(&stream, &filesystem)[0];
    EXPECT_EQ(0x400, inf.version);
    EXPECT_EQ("du5neU.Ld", inf.ld);
    EXPECT_EQ("DU5NEU.HIC", inf.primary.hic);
    EXPECT_EQ("du5neu.IDX", inf.primary.idx);
    EXPECT_EQ("du5neu.bof", inf.primary.bof);
    ASSERT_EQ(1, inf.resources.size());
    EXPECT_EQ("d5snd.fsi", inf.resources[0].fsi);
    EXPECT_EQ("d5snd.idx", inf.resources[0].idx);
    EXPECT_EQ("d5snd.Bof", inf.resources[0].bof);
}

TEST(duden, ParseInfFileFsiPackReferencesPrimaryPack) {
    FileStream stream(testPath("duden_testfiles/fsi-pack-references-primary-pack.inf"));
    TestFileSystem filesystem;
    auto inf = parseInfFile(&stream, &filesystem)[0];
    EXPECT_EQ(0x400, inf.version);
    EXPECT_EQ("du5neU.Ld", inf.ld);
    EXPECT_EQ("DU5NEU.HIC", inf.primary.hic);
    EXPECT_EQ("du5neu.IDX", inf.primary.idx);
    EXPECT_EQ("du5neu.bof", inf.primary.bof);
    ASSERT_EQ(0, inf.resources.size());
}

TEST(duden, ParseTwoWayDictInfFile) {
    FileStream stream(testPath("duden_testfiles/two-way-dict.inf"));
    TestFileSystem filesystem(true);
    auto infs = parseInfFile(&stream, &filesystem);
    ASSERT_EQ(2, infs.size());

    auto inf = infs[0];
    EXPECT_EQ(0x302, inf.version);
    EXPECT_EQ("DOEDK.HIC", inf.primary.hic);
    EXPECT_EQ("DOEDK.IDX", inf.primary.idx);
    EXPECT_EQ("DOEDK.BOF", inf.primary.bof);
    ASSERT_EQ(2, inf.resources.size());
    EXPECT_EQ("", inf.resources[0].fsi);
    EXPECT_EQ("dodekkey.IDX", inf.resources[0].idx);
    EXPECT_EQ("dodekkey.BOF", inf.resources[0].bof);
    EXPECT_EQ("", inf.resources[1].fsi);
    EXPECT_EQ("doedkkey.IDX", inf.resources[1].idx);
    EXPECT_EQ("doedkkey.BOF", inf.resources[1].bof);

    inf = infs[1];
    EXPECT_EQ(0x302, inf.version);
    EXPECT_EQ("DODEK.HIC", inf.primary.hic);
    EXPECT_EQ("DODEK.IDX", inf.primary.idx);
    EXPECT_EQ("DODEK.BOF", inf.primary.bof);
    ASSERT_EQ(2, inf.resources.size());
    EXPECT_EQ("", inf.resources[0].fsi);
    EXPECT_EQ("dodekkey.IDX", inf.resources[0].idx);
    EXPECT_EQ("dodekkey.BOF", inf.resources[0].bof);
    EXPECT_EQ("", inf.resources[1].fsi);
    EXPECT_EQ("doedkkey.IDX", inf.resources[1].idx);
    EXPECT_EQ("doedkkey.BOF", inf.resources[1].bof);
}

TEST(duden, ParseTwoWayDictInfFileWithoutSecondTitle) {
    FileStream stream(testPath("duden_testfiles/two-way-dict-no-title.inf"));
    TestFileSystem filesystem(true);
    auto infs = parseInfFile(&stream, &filesystem);
    ASSERT_EQ(2, infs.size());

    auto inf = infs[0];
    EXPECT_EQ(0x300, inf.version);
    EXPECT_EQ("CHWDE.LD", inf.ld);
    EXPECT_EQ("chwde.hic", inf.primary.hic);
    EXPECT_EQ("CHWDE.IDX", inf.primary.idx);
    EXPECT_EQ("CHWDE.BOF", inf.primary.bof);
    ASSERT_EQ(2, inf.resources.size());
    EXPECT_EQ("", inf.resources[0].fsi);
    EXPECT_EQ("CHWDEKEY.IDX", inf.resources[0].idx);
    EXPECT_EQ("CHWDEKEY.BOF", inf.resources[0].bof);
    EXPECT_EQ("", inf.resources[1].fsi);
    EXPECT_EQ("CHWEDKEY.IDX", inf.resources[1].idx);
    EXPECT_EQ("CHWEDKEY.BOF", inf.resources[1].bof);

    inf = infs[1];
    EXPECT_EQ(0x300, inf.version);
    EXPECT_EQ("CHWED.LD", inf.ld);
    EXPECT_EQ("chwed.hic", inf.primary.hic);
    EXPECT_EQ("CHWED.IDX", inf.primary.idx);
    EXPECT_EQ("CHWED.BOF", inf.primary.bof);
    ASSERT_EQ(2, inf.resources.size());
    EXPECT_EQ("", inf.resources[0].fsi);
    EXPECT_EQ("CHWDEKEY.IDX", inf.resources[0].idx);
    EXPECT_EQ("CHWDEKEY.BOF", inf.resources[0].bof);
    EXPECT_EQ("", inf.resources[1].fsi);
    EXPECT_EQ("CHWEDKEY.IDX", inf.resources[1].idx);
    EXPECT_EQ("CHWEDKEY.BOF", inf.resources[1].bof);
}

TEST(duden, ParseTwoWayDictInfFileWithSpacesAfterSemicolon) {
    FileStream stream(testPath("duden_testfiles/spaces_after_semicolon.inf"));
    TestFileSystem filesystem(true);
    filesystem.files() = {
        "FWMEDE.idX",
        "FWMEED.Idx"
    };
    auto infs = parseInfFile(&stream, &filesystem);
    ASSERT_EQ(2, infs.size());

    auto inf = infs[0];
    EXPECT_EQ(0x290, inf.version);
    EXPECT_EQ("FWMEED.LD", inf.ld);
    EXPECT_EQ("FWMEED.hic", inf.primary.hic);
    EXPECT_EQ("FWMEED.Idx", inf.primary.idx);
    EXPECT_EQ("FWMEED.BOF", inf.primary.bof);

    inf = infs[1];
    EXPECT_EQ(0x290, inf.version);
    EXPECT_EQ("FWMEDE.LD", inf.ld);
    EXPECT_EQ("FWMEDE.hic", inf.primary.hic);
    EXPECT_EQ("FWMEDE.idX", inf.primary.idx);
    EXPECT_EQ("FWMEDE.BOF", inf.primary.bof);
}

TEST(duden, ParseTwoWayDictInfFileWithFsdBlob) {
    FileStream stream(testPath("duden_testfiles/fsd_blob.inf"));
    TestFileSystem filesystem(true);
    auto infs = parseInfFile(&stream, &filesystem);
    ASSERT_EQ(2, infs.size());

    auto inf = infs[0];
    EXPECT_EQ(0x302, inf.version);
    EXPECT_EQ("TWED.LD", inf.ld);
    EXPECT_EQ("twed.hic", inf.primary.hic);
    EXPECT_EQ("TWED.IDX", inf.primary.idx);
    EXPECT_EQ("TWED.BOF", inf.primary.bof);
    ASSERT_EQ(1, inf.resources.size());
    EXPECT_EQ("TWEDAWAV.FSI", inf.resources[0].fsi);
    EXPECT_EQ("TWEDAWAV.FSD", inf.resources[0].fsd);

    inf = infs[1];
    EXPECT_EQ(0x302, inf.version);
    EXPECT_EQ("TWDE.LD", inf.ld);
    EXPECT_EQ("twde.hic", inf.primary.hic);
    EXPECT_EQ("TWDE.IDX", inf.primary.idx);
    EXPECT_EQ("TWDE.BOF", inf.primary.bof);
    ASSERT_EQ(1, inf.resources.size());
    EXPECT_EQ("TWEDAWAV.FSI", inf.resources[0].fsi);
    EXPECT_EQ("TWEDAWAV.FSD", inf.resources[0].fsd);
}

class TestFileSystem4 : public IFileSystem {
    std::string _ld = "KDU1NEU";
    CaseInsensitiveSet _files;

public:
    std::unique_ptr<dictlsd::IRandomAccessStream> open(std::filesystem::path) override {
        return std::make_unique<dictlsd::InMemoryStream>(_ld.c_str(), _ld.size());
    }

    CaseInsensitiveSet& files() override {
        return _files;
    }
};

TEST(duden, ParseInfWithInconsistenPrimaryFileNames) {
    FileStream stream(testPath("duden_testfiles/inconsistent_primary_file_names.inf"));
    TestFileSystem4 filesystem;
    auto infs = parseInfFile(&stream, &filesystem);
    ASSERT_EQ(1, infs.size());

    auto inf = infs[0];
    EXPECT_EQ(1, inf.version);
    EXPECT_EQ("DU1.LD", inf.ld);
    EXPECT_EQ("DU1NEU.HIC", inf.primary.hic);
    EXPECT_EQ("DU1.IDX", inf.primary.idx);
    EXPECT_EQ("DU1.BOF", inf.primary.bof);
}

TEST(duden, CaseInsensitiveFileSystemSearch) {
    CaseInsensitiveSet set{"123_abc.bin", "abc.bin", "file.ext"};
    auto found = set.find("ABC.bin");
    ASSERT_NE(end(set), found);
    ASSERT_EQ("abc.bin", found->string());
    found = set.find("file.eXT");
    ASSERT_NE(end(set), found);
    ASSERT_EQ("file.ext", found->string());
    found = set.find("123");
    ASSERT_EQ(end(set), found);
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

TEST(duden, UnicodeHandleEscapesInsideArticleRefs) {
    const uint8_t text[] = {
        0x74, 0x20, 0x5C, 0x53, 0x7B, 0x73, 0xA0, 0xE4, 0x75, 0x67, 0x65, 0x6E,
        0x3B, 0x3A, 0x30, 0x30, 0x34, 0x38, 0x37, 0x32, 0x32, 0x32, 0x32, 0x7D,
        0x2E, 0x00
    };
    auto utf = dudenToUtf8((const char*)text);
    ASSERT_EQ(u8"t \\S{säugen;:004872222}.", utf);
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

TEST(duden, UnicodeIgnoreEscapesInsideEscapeC) {
    const uint8_t text[] = {
        'a', '@', 'C', 'a', 0xf6, 'b', '\n', 'c', 0
    };
    auto utf = dudenToUtf8((const char*)text);
    ASSERT_EQ(u8"a@Caöb\nc", utf);

    const uint8_t text2[] = {
        'a', '@', 'C', 'a', 0xf6, 'b', 0
    };
    utf = dudenToUtf8((const char*)text2);
    ASSERT_EQ(u8"a@Caöb", utf);
}

TEST(duden, SimpleLdFileTest) {
    FileStream stream(testPath("duden_testfiles/simple.ld"));
    auto ld = parseLdFile(&stream);
    ASSERT_EQ("BTB", ld.baseFileName);
    ASSERT_EQ("Some thing", ld.name);
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
    auto text = read_all_text(testPath("duden_testfiles/article1"));
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

TEST(duden, ParseTextReferenceWithSelectionRange) {
    auto text = "\\S{ref;:06831017:05172-05835}abc";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  ReferencePlaceholderRun; code=; num=6831017; num2=-1; range=5172-5835\n"
                    "    TextRun\n"
                    "      PlainRun: ref\n"
                    "  PlainRun: abc\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ParseTextReferenceWithSelectionRangeWithoutEndOffset) {
    auto text = "\\S{ref;:00022110:00000000}abc";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  ReferencePlaceholderRun; code=; num=22110; num2=-1; range=0-0\n"
                    "    TextRun\n"
                    "      PlainRun: ref\n"
                    "  PlainRun: abc\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, ResolveArticleReference) {
    auto text = "\\S{Diskettenformat;:025004230}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream(testPath("duden_testfiles/simple.ld"));
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
    FileStream stream(testPath("duden_testfiles/simple.ld"));
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
    FileStream stream(testPath("duden_testfiles/simple.ld"));
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
    FileStream stream(testPath("duden_testfiles/simple.ld"));
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

TEST(duden, ParseInlineImageReference) {
    auto text = "a\\S{;.Ieuro.bmp}b";
    ParsingContext context;
    auto expected = "TextRun\n"
                    "  PlainRun: a\n"
                    "  ReferencePlaceholderRun; code=Ieuro.bmp; num=-1; num2=-1\n"
                    "    TextRun\n"
                    "  PlainRun: b\n";
    auto run = parseDudenText(context, text);
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, ResolveInlineImageReference) {
    auto text = "\\S{;.Ieuro.bmp;T}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream(testPath("duden_testfiles/simple.ld"));
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  InlineImageRun; name=euro.bmp; secondary=\n";
    ASSERT_EQ(expected, tree);
    ASSERT_EQ(u8"[s]euro.bmp[/s]", printDsl(run));
}

TEST(duden, ResolveAudioSReference) {
    auto text = u8"\\S{;.Ispeaker.bmp;T;à la longue.Adp}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream(testPath("duden_testfiles/simple.ld"));
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    auto tree = printTree(run);
    auto expected = u8"TextRun\n"
                    u8"  InlineImageRun; name=speaker.bmp; secondary=à la longue.wav\n";
    ASSERT_EQ(expected, tree);
    ASSERT_EQ(u8"[s]à la longue.wav[/s]", printDsl(run));
}

TEST(duden, ResolveAudioWReference) {
    const uint8_t encoded[] = {
        '\\', 'w', '{', 'a', 'b', 'g', 'e', 'w', 0xf6, 'h',
        'n', 'e', 'n', '_', '9', '2', '.', 'a', 'd', 'p', '}', 0
    };
    auto text = dudenToUtf8(std::string(reinterpret_cast<const char*>(encoded)));
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream(testPath("duden_testfiles/simple.ld"));
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    resolveReferences(context, run, ld, nullptr);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  InlineSoundRun; name=[(abgewöhnen_92.wav)]\n"
                    "    TextRun\n"
                    "      PlainRun: \n";
    ASSERT_EQ(expected, tree);
    ASSERT_EQ(u8"[s]abgewöhnen_92.wav[/s] ", printDsl(run));
}

TEST(duden, ResolveAudioWReference2) {
    auto text = "\\w{AE000001.adp \"AAA\";BE000001.adp \"BBB\";CC000001.adp \"CCC\"}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream(testPath("duden_testfiles/simple.ld"));
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  InlineSoundRun; name=[(AE000001.wav), (BE000001.wav), (CC000001.wav)]\n"
                    "    TextRun\n"
                    "      PlainRun:  \"AAA\"\n"
                    "    TextRun\n"
                    "      PlainRun:  \"BBB\"\n"
                    "    TextRun\n"
                    "      PlainRun:  \"CCC\"\n";

    ASSERT_EQ(expected, tree);
    ASSERT_EQ("[s]AE000001.wav[/s] \"AAA\", [s]BE000001.wav[/s] \"BBB\", [s]CC000001.wav[/s] \"CCC\" ", printDsl(run));
}

TEST(duden, ResolveAudioWReference3) {
    auto text = "\\w{AE000001.adp \"A@2AA@0@2\";BE000001.adp \"BBB\"}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream(testPath("duden_testfiles/simple.ld"));
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  InlineSoundRun; name=[(AE000001.wav), (BE000001.wav)]\n"
                    "    TextRun\n"
                    "      PlainRun:  \"A\n"
                    "      ItalicFormattingRun\n"
                    "        PlainRun: AA\n"
                    "      ItalicFormattingRun\n"
                    "        PlainRun: \"\n"
                    "    TextRun\n"
                    "      PlainRun:  \"BBB\"\n";
    ASSERT_EQ(expected, tree);
    ASSERT_EQ("[s]AE000001.wav[/s] \"A[i]AA[/i][i]\"[/i], [s]BE000001.wav[/s] \"BBB\" ", printDsl(run));
}

class TestFileSystem3 : public IFileSystem {
    CaseInsensitiveSet _files;

public:
    TestFileSystem3() {
        std::filesystem::path root = "";
        _files = {std::filesystem::u8path(u8"123.bmp"),
                  std::filesystem::u8path(u8"euro.bmp"),
                  std::filesystem::u8path(u8"АбfD.BMP"),
                  std::filesystem::u8path(u8"UNABKöMMLICH1V.WAV")};
    }
     std::unique_ptr<dictlsd::IRandomAccessStream> open(std::filesystem::path) override {
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
    FileStream stream(testPath("duden_testfiles/simple.ld"));
    auto ld = parseLdFile(&stream);
    TestFileSystem3 fs;
    resolveReferences(context, run, ld, &fs);
    ASSERT_EQ(u8"[s]АбfD.BMP[/s]", printDsl(run));
}

TEST(duden, ResolveInlineImageReferenceInconstistentCase2) {
    auto text = u8"\\S{;.Ispeaker.bmp;T;unabkömmlich1v.adp}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream(testPath("duden_testfiles/simple.ld"));
    auto ld = parseLdFile(&stream);
    TestFileSystem3 fs;
    resolveReferences(context, run, ld, &fs);
    ASSERT_EQ(u8"[s]UNABKöMMLICH1V.WAV[/s]", printDsl(run));
}

TEST(duden, InlinePictureReference) {
    auto text = "\\S{;.MBB:620000166;Caption}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream(testPath("duden_testfiles/simple.ld"));
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);

    ResourceFiles files;
    files["btb_pic"] = std::make_unique<std::ifstream>(testPath("duden_testfiles/duden_encoded_pic"), std::ios::binary);
    inlineReferences(context, run, files);

    auto picture = dynamic_cast<PictureReferenceRun*>(run->runs().front());
    ASSERT_NE(nullptr, picture);
    auto description = dynamic_cast<PlainRun*>(picture->description()->runs().front());
    ASSERT_NE(nullptr, description);

    ASSERT_EQ(u8"© Bibliographisches Institut & F. A. Brockhaus, Mannheim", picture->copyright());
    ASSERT_EQ(u8"b5bi1607.BMP", picture->imageFileName());
    ASSERT_EQ(u8"Die vom Fahrstrahl in gleichen Zeitintervallen überstrichenen Flächen (rosa) sind gleich groß", description->text());
}

TEST_F(duden_qt, ParseTextTable1) {
    auto text = read_all_text(testPath("duden_testfiles/table1"));
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

    auto html = printHtml(run);
    auto vec = renderHtml(html);
    std::ofstream f("/tmp/table.bmp");
    f.write((char*)&vec[0], vec.size());
}

TEST(duden, ParseTextTableWithNestedTable) {
    auto text = read_all_text(testPath("duden_testfiles/table_in_table_cell"));
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
                    "      PlainRun: OuterTable\n"
                    "      SoftLineBreakRun\n"
                    "      TableRun\n"
                    "        TableTag 1; from=1; to=-1\n"
                    "        TableTag 2; from=1; to=-1\n"
                    "        TableTag 3; from=-1; to=-1\n"
                    "        SoftLineBreakRun\n"
                    "        TableCellRun\n"
                    "          PlainRun: InnerTable\n"
                    "          SoftLineBreakRun\n"
                    "      SoftLineBreakRun\n"
                    "  SoftLineBreakRun\n";
    ASSERT_EQ(expected, tree);

    auto outerTable = dynamic_cast<TableRun*>(run->runs().front());
    ASSERT_NE(nullptr, outerTable->table());
    ASSERT_EQ(1, outerTable->table()->rows());
    ASSERT_EQ(1, outerTable->table()->columns());

    auto cell = outerTable->table()->cell(0, 0);
    expected = "    TableCellRun\n"
               "      PlainRun: OuterTable\n"
               "      SoftLineBreakRun\n"
               "      TableRun\n"
               "        TableTag 1; from=1; to=-1\n"
               "        TableTag 2; from=1; to=-1\n"
               "        TableTag 3; from=-1; to=-1\n"
               "        SoftLineBreakRun\n"
               "        TableCellRun\n"
               "          PlainRun: InnerTable\n"
               "          SoftLineBreakRun\n"
               "      SoftLineBreakRun\n";

    ASSERT_EQ(expected, printTree(cell));

    auto innerTable = dynamic_cast<TableRun*>(cell->runs().at(2));
    ASSERT_NE(nullptr, innerTable);
    ASSERT_NE(nullptr, innerTable->table());
    ASSERT_EQ(1, innerTable->table()->rows());
    ASSERT_EQ(1, innerTable->table()->columns());
    ASSERT_EQ("InnerTable ", printDsl(innerTable->table()->cell(0, 0)));
}

TEST(duden, ParseTextTableWithExtraCells) {
    auto text = read_all_text(testPath("duden_testfiles/table_several_extra_cells_and_footer"));
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
    auto text = read_all_text(testPath("duden_testfiles/table_curly_braces"));
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
    auto text = read_all_text(testPath("duden_testfiles/table_merged_cells"));
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
    FileStream stream(testPath("duden_testfiles/simple.ld"));
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);

    ResourceFiles files;
    files["btb_tab"] = std::make_unique<std::ifstream>(testPath("duden_testfiles/tab_file"), std::ios::binary);
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
                    "  ColorFormattingRun; rgb=0000ff; name=blue; tilde=0\n"
                    "    PlainRun: abc\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, ParseColorsWithoutSpaces) {
    std::string text = "\\F{_FF0000}abc\\F{FF0000_}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  ColorFormattingRun; rgb=ff0000; name=red; tilde=0\n"
                    "    PlainRun: abc\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, ParseColorsWithoutSpacesAndUnderscore) {
    std::string text = "\\F{~FF0000}abc\\F{FF0000~}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  ColorFormattingRun; rgb=ff0000; name=red; tilde=1\n"
                    "    PlainRun: abc\n";
    ASSERT_EQ(expected, printTree(run));
}

TEST(duden, SimpleFormatting) {
    std::string text = "\\F{_FF 00 00}color\\F{FF 00 00_}"
                       "\\F{~FF0000}color\\F{FF0000~}"
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
                    "color"
                    "[b]bold[/b]"
                    "[b][i]bold italic[/i][/b]"
                    "[i]italic[/i]"
                    "[b]bold [u]underline[/u][/b]"
                    "plain (a[u]d[/u]d)"
                    "plain (a[u]d[/u]d)"
                    "plain a[sub]sub[/sub] and a[sup]sup[/sup]";
    ASSERT_EQ(expected, printDsl(run));
    expected = "<font color=\"red\">color</font>"
               "color"
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

TEST(duden, ConvertWebLinkToDsl) {
    std::string text = "\\F{_WebLink}abc\\F{WebLink_}de";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    ASSERT_EQ("[url]abc[/url]de", duden::printDsl(run));
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

TEST(duden, ParseIdWithTrailingPercentSign) {
    std::string text = "a@C%ID=12345%\nb";
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

TEST(duden, ParseUnterminatedReferences) {
    std::string text = "\\S{Abc.\\\\\n@1abc";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    ASSERT_EQ("", printDsl(run));
}

TEST(duden, HandleNewLinesInDsl) {
    ParsingContext context;
    auto run = parseDudenText(context, "a\\\\b");
    ASSERT_EQ("a[br]\nb", printDsl(run));
    run = parseDudenText(context, "a\nb");
    ASSERT_EQ("a b", printDsl(run));
}

TEST(duden, HandleAddendumInDsl) {
    ParsingContext context;
    auto run = parseDudenText(context, "Abc \\F{_ADD}123\\F{ADD_}");
    ASSERT_EQ("Abc (123)", printDsl(run));
}

TEST(duden, HandleAlignmentInDsl) {
    ParsingContext context;
    auto run = parseDudenText(context, "\\F{_Right}abc\\F{Right_}");
    ASSERT_EQ("abc", printDsl(run));
    run = parseDudenText(context, "\\F{_Left}abc\\F{Left_}");
    ASSERT_EQ("abc", printDsl(run));
    run = parseDudenText(context, "\\F{_Center}abc\\F{Center_}");
    ASSERT_EQ("abc", printDsl(run));
}

TEST(duden, EscapeSquareBracketsInDsl) {
    ParsingContext context;
    auto run = parseDudenText(context, "[abc]");
    ASSERT_EQ("\\[abc\\]", printDsl(run));
}

TEST(duden, PrintInlineRenderedTable) {
    auto text = read_all_text(testPath("duden_testfiles/table1"));
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto tableRun = dynamic_cast<TableRun*>(run->runs().front());
    ASSERT_NE(nullptr, tableRun);
    tableRun->setRenderedName("table.bmp");
    ASSERT_EQ("\n[s]table.bmp[/s] ", printDsl(run));
}

TEST(duden, InlineRenderAndPrintPicture) {
    auto text = "\\S{;.MBB:620000166;Caption}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream(testPath("duden_testfiles/simple.ld"));
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    ResourceFiles files;
    files["btb_pic"] = std::make_unique<std::ifstream>(testPath("duden_testfiles/duden_encoded_pic"), std::ios::binary);
    inlineReferences(context, run, files);
    std::string name;
    TableRenderer renderer([&](auto, auto n) { name = n; }, [](auto){return std::vector<char>();});
    renderer.render(run);
    auto expected = "\n----------\n"
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

    article = parseDudenText(context, "    \n@1Heading @0\nAnd then body");
    res = dedupHeading(heading, article);
    ASSERT_EQ(true, res);
    ASSERT_EQ("And then body", printDsl(article));

    article = parseDudenText(context, "@1Heading: @0@C%ID=0000\nAnd then body");
    res = dedupHeading(heading, article);
    ASSERT_EQ(true, res);
    ASSERT_EQ("And then body", printDsl(article));
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

TEST(duden, ParseCEscapeAtEOL) {
    ParsingContext context;
    auto run = parseDudenText(context, "@9@C%");
    ASSERT_EQ("", printDsl(run));
}

TEST(duden, IgnoreUnknownEscapes) {
    ParsingContext context;
    auto run = parseDudenText(context, "a@bc");
    ASSERT_EQ("abc", printDsl(run));
    run = parseDudenText(context, "\\00520FE");
    ASSERT_EQ("00520FE", printDsl(run));
}

TEST(duden, IgnoreCurlyBraces) {
    ParsingContext context;
    auto run = parseDudenText(context, "abc {...} def");
    ASSERT_EQ("abc {...} def", printDsl(run));
}

TEST_F(duden_qt, InlineRenderAndPrintTable) {
    auto text = "\\S{Tabelle: table name;.MT:660000000;Tabelle}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    FileStream stream(testPath("duden_testfiles/simple.ld"));
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    ResourceFiles files;
    files["btb_tab"] = std::make_unique<std::ifstream>(testPath("duden_testfiles/tab_file"), std::ios::binary);
    inlineReferences(context, run, files);
    std::string name;
    TableRenderer renderer([&](auto, auto n) { name = n; }, [](auto){return std::vector<char>();});
    renderer.render(run);
    auto expected = bformat(
                "\n----------\n"
                "Tabelle: table name\n"
                "[b]Table: Name[/b] \n"
                "[s]%s[/s] "
                "Footer 1[br]\n"
                "Footer 2[br]\n[br]\n"
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
    std::string text = "\\S{here comes the semicolon; and text continues;.MT:660042728;Tabelle}abc";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto expected = "TextRun\n"
                    "  ReferencePlaceholderRun; code=; num=-1; num2=-1\n"
                    "    TextRun\n"
                    "      PlainRun: here comes the semicolon\n"
                    "    PlainRun: ; and text continues\n"
                    "    PlainRun: .MT:660042728\n"
                    "    PlainRun: Tabelle\n"
                    "  PlainRun: abc\n";
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
    FileStream stream(testPath("duden_testfiles/simple.ld"));
    auto ld = parseLdFile(&stream);
    resolveReferences(context, run, ld, nullptr);
    ResourceFiles files;
    files["btb_tab"] = std::make_unique<std::ifstream>(testPath("duden_testfiles/tab_file_embedded_image"), std::ios::binary);
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
    resolveArticleReferences(run, [](auto offset, [[maybe_unused]] auto& hint) {
        if (offset == 620083)
            return "ResolvedName";
        if (offset == 620084)
            return "SameName";
        return "error";
    });
    ASSERT_EQ("[ref]ResolvedName[/ref] (ArticleName)[ref]SameName[/ref]", printDsl(run));
}

TEST(duden, InlineArticleReferenceHandleSpaces) {
    ParsingContext context;
    auto run = parseDudenText(context,
                              "\\S{ArticleName ;:000620083}"
                              "\\S{SameName ;:000620084}"
                              "\\S{ArticleName, ;:000620083}"
                              "\\S{SameName, ;:000620084}"
                              "\\S{. SameName, ;:000620084}"
                              "\\S{Short, ;:000620085}");
    resolveReferences(context, run, {}, nullptr);
    resolveArticleReferences(run, [](auto offset, [[maybe_unused]] auto& hint) {
        if (offset == 620083)
            return "ResolvedName";
        if (offset == 620084)
            return "SameName";
        if (offset == 620085)
            return "VeryLongName";
        return "error";
    });
    ASSERT_EQ("[ref]ResolvedName[/ref] (ArticleName) "
              "[ref]SameName[/ref] "
              "[ref]ResolvedName[/ref] (ArticleName), "
              "[ref]SameName[/ref], "
              ". [ref]SameName[/ref], "
              "[ref]VeryLongName[/ref] (Short), ",
              printDsl(run));
}

TEST(duden, InlineArticleReferenceWithAliasEndingWithSpace) {
    ParsingContext context;
    auto run = parseDudenText(context, "\\S{ArticleName ;:000620083}text");
    resolveReferences(context, run, {}, nullptr);
    resolveArticleReferences(run, [](auto offset, [[maybe_unused]] auto& hint) {
        if (offset == 620083)
            return "ResolvedName";
        return "error";
    });
    ASSERT_EQ("[ref]ResolvedName[/ref] (ArticleName) text", printDsl(run));
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

TEST(duden, GroupHicEntries7) {
    std::vector<HicLeaf> leafs {
        HicLeaf{"Adolf Brunner $$$$  16 27114624 -16", HicEntryType::Reference, 0x001e810a},
        HicLeaf{"Adolf Busch $$$$ 14 620023071 1005 0", HicEntryType::Reference, 0x001e810b},
        HicLeaf{"Adolf Butenandt $$$$ 55 620023184 1005 0", HicEntryType::Reference, 0x001e810d},
        HicLeaf{"Alf Daens $$$$  -1 37310711 -16", HicEntryType::Reference, 0x001e810f},
    };
    auto groups = groupHicEntries(leafs);
    ASSERT_EQ(2, groups.size());
    ASSERT_EQ((std::vector{"Adolf Brunner"s}), groups[27114623].headings);
    ASSERT_EQ((std::vector{"Alf Daens"s}), groups[37310710].headings);
}

TEST(duden, HandleNewLinesInHtml) {
    auto text = "a\\\\b\nc";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto html = printHtml(run);
    ASSERT_EQ("a<br>bc", html);
}

TEST(duden, IgnoreZeroesInDecodedText) {
    uint8_t data[] {
        0xA2, 0xA5, 0xA4, 0x55, 0xA2, 0xAD, 0xA4, 0x56, 0x5C, 0
    };
    auto decoded = dudenToUtf8(reinterpret_cast<char*>(data));
    ASSERT_EQ(u8"a͜i\\", decoded);
}

TEST(duden, TreatLowerSEscapeAsItalic) {
    std::string text = "\\s{abc}";
    ParsingContext context;
    auto run = parseDudenText(context, text);
    auto tree = printTree(run);
    auto expected = "TextRun\n"
                    "  ItalicFormattingRun\n"
                    "    PlainRun: abc\n";
    ASSERT_EQ(expected, tree);
}

TEST(duden, GuessLanguages) {
    auto deuCode = 1031;
    auto engCode = 1033;

    LdFile ld1, ld2;
    auto set = [] (auto& ld, auto source) {
        ld.sourceLanguage = source;
        ld.sourceLanguageCode = -1;
        ld.targetLanguageCode = -1;
    };

    set(ld1, "deu");
    duden::updateLanguageCodes({&ld1});
    EXPECT_EQ(deuCode, ld1.sourceLanguageCode);
    EXPECT_EQ(deuCode, ld1.targetLanguageCode);

    set(ld1, "enu");
    duden::updateLanguageCodes({&ld1});
    EXPECT_EQ(engCode, ld1.sourceLanguageCode);
    EXPECT_EQ(deuCode, ld1.targetLanguageCode);

    set(ld1, "deu");
    set(ld2, "enu");
    duden::updateLanguageCodes({&ld1, &ld2});
    EXPECT_EQ(deuCode, ld1.sourceLanguageCode);
    EXPECT_EQ(engCode, ld1.targetLanguageCode);
    EXPECT_EQ(engCode, ld2.sourceLanguageCode);
    EXPECT_EQ(deuCode, ld2.targetLanguageCode);
}

TEST(duden, CollapseNewLinesInDsl) {
    ParsingContext context;
    auto run = parseDudenText(context, "@1Heading @0@C%ID=123\n\\\\\n@8\\\\\n@9\nabc\n123");
    ASSERT_EQ("[b]Heading [/b][br]\n[br]\nabc 123", printDsl(run));

    run = parseDudenText(context, "\n\n\n");
    ASSERT_EQ("", printDsl(run));

    run = parseDudenText(context, ".\n\n\n");
    ASSERT_EQ(". ", printDsl(run));

    run = parseDudenText(context, ".\n\\\\\\\n\n");
    ASSERT_EQ(".[br]\n", printDsl(run));

    run = parseDudenText(context, ".\n\nabc\n\\\\123");
    ASSERT_EQ(". abc[br]\n123", printDsl(run));

    run = parseDudenText(context, "1\\\\2\n\n\n");
    ASSERT_EQ("1[br]\n2 ", printDsl(run));

    run = parseDudenText(context, "@1abc@0\\\\\n@C%ID=000000001\n@0\\.\\\\\n");
    ASSERT_EQ("[b]abc[/b][br]\n.[br]\n", printDsl(run));

    run = parseDudenText(context, "@C%ID=000000001\n@0\\.\\\\\n");
    ASSERT_EQ(".[br]\n", printDsl(run));
}

TEST(duden, EscapeDirectivesInDsl) {
    ParsingContext context;
    auto run = parseDudenText(context, "#^@@");
    ASSERT_EQ("\\#\\^\\@", printDsl(run));
}

TEST(duden, HandleNonBreakingSpace) {
    ParsingContext context;
    auto run = parseDudenText(context, "a~b");
    ASSERT_EQ("a\xC2\xA0""b", printDsl(run));
}

TEST(duden, HandleTildeInDsl) {
    ParsingContext context;
    auto run = parseDudenText(context, "@~");
    ASSERT_EQ("\\~", printDsl(run));
}

TEST(duden, DefaultArticleResolveHint) {
    std::map<int32_t, HeadingGroup> groups;
    groups[10] = {
        {"a", "b", "c"}
    };
    ParsingContext context;
    ASSERT_EQ("a", defaultArticleResolve(groups, 11, "z", context));
    ASSERT_EQ("b", defaultArticleResolve(groups, 11, "b", context));
    ASSERT_EQ("b", defaultArticleResolve(groups, 11, "b ", context));
    ASSERT_EQ("b", defaultArticleResolve(groups, 11, "b,", context));
    ASSERT_EQ("", defaultArticleResolve(groups, 15, "z", context));
}

TEST(duden, EscapeParenthesesInDslHeadings) {
    ParsingContext context;
    auto run = parseDudenText(context, "Heading (not a variant)");
    ASSERT_EQ("Heading \\(not a variant\\)", printDslHeading(run));
}

TEST(duden, SkipIncorrectlyEncodedChars) {
    ParsingContext context;
    ASSERT_EQ("a?1234", dudenToUtf8("a\xFC""1234"));
}
