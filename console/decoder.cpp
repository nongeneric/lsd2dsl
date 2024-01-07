#include "common/DslWriter.h"
#include "common/Log.h"
#include "common/ZipWriter.h"
#include "common/bformat.h"
#include "common/overloaded.h"
#include "common/version.h"
#include "common/WavWriter.h"
#include "lingvo/LSAReader.h"
#include "lingvo/lsd.h"
#include "lingvo/tools.h"
#include "lingvo/WriteDsl.h"

#ifdef ENABLE_DUDEN
#include <QApplication>
#include "duden/AdpDecoder.h"
#include "duden/Dictionary.h"
#include "duden/Duden.h"
#include "duden/HtmlRenderer.h"
#include "duden/InfFile.h"
#include "duden/Writer.h"
#include "duden/text/Parser.h"
#include "duden/text/Printers.h"
#include "duden/text/TextRun.h"
#endif

#include <boost/program_options.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <string_view>

namespace po = boost::program_options;
using namespace lingvo;

struct Entry {
    std::u16string heading;
    std::u16string article;
};

int parseLSD(std::filesystem::path lsdPath,
             std::filesystem::path outputPath,
             int sourceFilter,
             int targetFilter,
             bool dumb,
             Log& log)
{
    common::FileStream ras(lsdPath);
    common::BitStreamAdapter bstr(&ras);
    LSDDictionary reader(&bstr);
    LSDHeader header = reader.header();

    log.regular("Version:  %x", header.version);
    log.regular("Headings: %d", header.entriesCount);
    log.regular("Source:   %d (%s)", header.sourceLanguage, toUtf8(langFromCode(header.sourceLanguage)));
    log.regular("Target:   %d (%s)", header.targetLanguage, toUtf8(langFromCode(header.targetLanguage)));
    log.regular("Name:     %s", toUtf8(reader.name()));

    if (!reader.supported()) {
        log.regular("Unsupported dictionary version");
        log.regular("Only the following versions are implemented:");
        log.regular("  Lingvo x6: 151005, 152001, 155001");
        log.regular("  Lingvo x5: 141004, 142001, 145001");
        log.regular("  Lingvo x5: 141004, 142001, 145001");
        log.regular("  Lingvo x3: 131001, 132001");
        log.regular("  Lingvo 12: 120001");
        log.regular("  Lingvo 11: 110001");
        return 1;
    }

    if ((sourceFilter != -1 && sourceFilter != header.sourceLanguage) ||
        (targetFilter != -1 && targetFilter != header.targetLanguage))
    {
        log.regular("ignoring");
        return 0;
    }

    if (!outputPath.empty()) {
        lingvo::writeDSL(&reader, lsdPath.filename(), outputPath, dumb, log);
    }

    return 0;
}

#ifdef ENABLE_DUDEN
int printDudenInfo(std::filesystem::path infPath, Log& log) {
    duden::FileSystem fs(infPath.parent_path());
    common::FileStream infStream(infPath);

    auto infs = duden::parseInfFile(&infStream, &fs);
    for (size_t i = 0; i < infs.size(); ++i) {
        duden::Dictionary dict(&fs, infPath, i);
        log.regular("Dictionary: %s", dict.ld().name);
        log.regular("  Version: %x", dict.inf().version);

        auto ld = dict.ld();
        log.regular("  Source: %s", ld.sourceLanguage);
        log.regular("  Primary:");
        log.regular("    BOF: %s", dict.inf().primary.bof);
        log.regular("    HIC: %s", dict.inf().primary.hic);
        log.regular("    IDX: %s", dict.inf().primary.idx);
        for (auto& resource : dict.inf().resources) {
            log.regular("  Resource:");
            log.regular("    BOF: %s", resource.bof);
            if (!resource.fsi.empty()) {
                log.regular("    FSI: %s", resource.fsi);
            }
            log.regular("    IDX: %s", resource.idx);
        }
    }
    return 0;
}

int parseDuden(std::filesystem::path infPath, std::filesystem::path outputPath, Log& log) {
    common::FileStream infStream(infPath);
    duden::FileSystem fs(infPath.parent_path());
    auto infs = duden::parseInfFile(&infStream, &fs);

    for (size_t i = 0; i < infs.size(); ++i) {
        if (!infs[i].supported) {
            log.regular("Unsupported dictionary version");
            return 1;
        }

        if (!outputPath.empty()) {
            duden::writeDSL(infPath, outputPath, i, log);
        }
    }

    return 0;
}

void decodeBofIdx(std::filesystem::path bofPath,
                  std::filesystem::path idxPath,
                  std::filesystem::path fsiPath,
                  bool dudenEncoding,
                  std::filesystem::path output) {
    common::FileStream fIdx(idxPath);
    auto fBof = std::make_shared<common::FileStream>(bofPath);
    duden::Archive archive(&fIdx, fBof);
    std::vector<char> vec;

    if (fsiPath.empty()) {
        archive.read(0, -1, vec);
        auto file = openForWriting(output / "decoded.dump");
        if (dudenEncoding) {
            auto text = duden::dudenToUtf8(std::string{begin(vec), end(vec)});
            file.write(text.data(), text.size());
        } else {
            file.write(vec.data(), vec.size());
        }
    } else {
        common::FileStream fFsi(fsiPath);
        auto entries = duden::parseFsiFile(&fFsi);
        for (auto& entry : entries) {
            auto file = openForWriting(output / entry.name);
            std::vector<char> vec;
            archive.read(entry.offset, entry.size, vec);
            file.write(vec.data(), vec.size());
        }
    }
}

void decodeHic(std::filesystem::path hicPath,
               std::filesystem::path output) {
    common::FileStream fHic(hicPath);
    auto hic = duden::parseHicFile(&fHic);
    auto f = openForWriting(output / "hic.dump");
    int leafNum = 0;
    std::vector<duden::HicLeaf> leafs;
    auto print = [&](auto& print, auto page, int level) -> void {
        std::string space(level * 4, ' ');
        f << bformat("%spage %x\n", space, page->offset);
        for (auto& entry : page->entries) {
            std::visit(overloaded{
                [&](duden::HicLeaf& leaf) {
                    f << bformat("%s%04d LEAF %02d  %08x  %s\n",
                               space, leafNum++,
                               static_cast<int>(leaf.type),
                               leaf.textOffset,
                               leaf.heading);
                    leafs.push_back(leaf);
                },
                [&](duden::HicNode& node) {
                    f << bformat("%sNODE %d (%x)  %d  %s\n",
                               space,
                               node.delta,
                               -node.delta,
                               node.count,
                               node.heading);
                    print(print, node.page, level + 1);
                }}, entry);
        }
    };

    print(print, hic.root, 0);

    f << "\ngroups:\n\n";
    for (auto& [offset, info] : duden::groupHicEntries(leafs)) {
        f << bformat("%08x, %d\n", offset, info.articleSize);
        for (auto& heading : info.headings) {
            f << heading << "\n";
        }
        f << "\n";
    }
}

void parseDudenText(std::filesystem::path textPath, std::filesystem::path output, bool dudenEncoding) {
    duden::ParsingContext context;
    auto f = openForReading(textPath);
    if (!f.is_open())
        throw std::runtime_error("can't open file");
    f.seekg(0, std::ios_base::end);
    std::string text(f.tellg(), ' ');
    f.seekg(0);
    f.read(text.data(), text.size());
    if (dudenEncoding) {
        text = duden::dudenToUtf8(text);
    }
    auto run = duden::parseDudenText(context, text);
    auto of = openForWriting(output / "textdump");
    auto tree = duden::printTree(run);
    of << tree << "\n\n";
    auto dsl = duden::printDsl(run);
    of << dsl << "\n\n";
    auto html = duden::printHtml(run);
    of << html << "\n\n";
}

void parseFsi(std::filesystem::path fsiPath, std::filesystem::path output) {
    common::FileStream fFsi(fsiPath);
    auto entries = duden::parseFsiFile(&fFsi);
    auto file = openForWriting(output / "fsi.dump");
    for (auto& entry : entries) {
        file << bformat("%08x  %08x  %s\n", entry.offset, entry.size, entry.name);
    }
}

void decodeAdp(std::filesystem::path adpPath, std::filesystem::path outputPath) {
    auto input = openForReading(adpPath);
    auto output = openForWriting(outputPath / "decoded.wav");
    input.seekg(0, std::ios_base::end);
    std::vector<char> inputVec(input.tellg());
    input.seekg(0);
    input.read(inputVec.data(), inputVec.size());
    std::vector<int16_t> pcmVec;;
    duden::decodeAdp(inputVec, pcmVec);
    std::vector<char> wav;
    common::createWav(pcmVec, wav, duden::ADP_SAMPLE_RATE, duden::ADP_CHANNELS);
    output.write(reinterpret_cast<char*>(wav.data()), wav.size());
}
#endif

class ConsoleLog : public Log {
    bool _verbose;
    bool _lastProgress = false;
    std::string _bar;
    std::string _empty;
    std::string _progressName;

protected:
    void reportLog(std::string line, bool verbose) override {
        if (_verbose || !verbose) {
            if (_lastProgress) {
                fmt::print("\n");
            }
            fmt::print("{}\n", line);
            _lastProgress = false;
        }
    }

    void reportProgress(int percentage) override {
        int left = static_cast<int>(percentage / 100.f * _bar.size());
        int right = _bar.size() - left;
        fmt::print("{}",
                   bformat("\r%s %3d%% [%s%s]",
                           _progressName,
                           percentage,
                           std::string_view(_bar).substr(0, left),
                           std::string_view(_empty).substr(0, right)));
        if (percentage == 100) {
            fmt::print("\n");
            std::fflush(stdout);
            _lastProgress = false;
        } else {
            std::fflush(stdout);
            _lastProgress = true;
        }
    }

    void reportProgressReset(std::string name) override {
        if (_lastProgress) {
            fmt::print("\n");
            std::fflush(stdout);
        }
        _progressName = name;
        _lastProgress = false;
    }

public:
    ConsoleLog(bool verbose) : _verbose(verbose), _bar(50, '='), _empty(_bar.size(), ' ') {}
};

int lsd2dsl_main(int argc, char* argv[]) {
#ifdef ENABLE_DUDEN
    QApplication a(argc, argv);
    bool dudenEncoding, dudenPrintInfo;
#endif
    std::string lsdPathStr, lsaPathStr, dudenPathStr, outputPathStr;
    std::string bofPathStr, idxPathStr, fsiPathStr, hicPathStr, adpPathStr, textPathStr;
    int sourceFilter = -1, targetFilter = -1;
    bool isDumb, verbose;
    po::options_description console_desc("Allowed options");
    try {
        console_desc.add_options()
            ("help", "produce help message")
            ("lsd", po::value(&lsdPathStr), "LSD dictionary to decode")
#ifdef ENABLE_DUDEN
            ("duden", po::value(&dudenPathStr), "Duden dictionary to decode (.inf file)")
#endif
            ("lsa", po::value(&lsaPathStr), "LSA sound archive to decode")
            ("source-filter", po::value<int>(&sourceFilter),
                "ignore dictionaries with source language != source-filter")
            ("target-filter", po::value<int>(&targetFilter),
                "ignore dictionaries with target language != target-filter")
            ("codes", "print supported languages and their codes")
            ("out", po::value(&outputPathStr), "output directory")
            ("dumb", "don't combine variant headings and headings "
                     "referencing the same article")
            ("verbose", "verbose logging")
            ("version", "print version")
            ;

        if (g_debug) {
            console_desc.add_options()
                ("bof", po::value(&bofPathStr), "Duden BOF file path")
                ("idx", po::value(&idxPathStr), "Duden IDX file path")
                ("fsi", po::value(&fsiPathStr), "Duden FSI file path")
                ("hic", po::value(&hicPathStr), "Duden HIC file path")
                ("adp", po::value(&adpPathStr), "Duden ADP file path")
                ("text", po::value(&textPathStr), "Duden decoded text file path")
                ("duden-info", "Print dictionary info and return")
                ("duden-utf", "Decode Duden to Utf");
        }

        po::variables_map console_vm;
        po::store(po::parse_command_line(argc, argv, console_desc), console_vm);
        if (console_vm.count("help")) {
            fmt::print("{}", fmt::streamed(console_desc));
            return 0;
        }
        if (console_vm.count("codes")) {
            printLanguages();
            return 0;
        }
        if (console_vm.count("version")) {
            fmt::print("lsd2dsl version {}\n", g_version);
            return 0;
        }
        isDumb = console_vm.count("dumb");
        verbose = console_vm.count("verbose");
#ifdef ENABLE_DUDEN
        dudenEncoding = console_vm.count("duden-utf");
        dudenPrintInfo = console_vm.count("duden-info");
#endif
        po::notify(console_vm);
    } catch(std::exception& e) {
        fmt::print("can't parse program options:\n{}\n\n{}", e.what(), fmt::streamed(console_desc));
        return 1;
    }

    if (lsdPathStr.empty() && lsaPathStr.empty() && dudenPathStr.empty() && bofPathStr.empty() &&
        textPathStr.empty() && hicPathStr.empty() && fsiPathStr.empty() && adpPathStr.empty()) {
        fmt::print("{}", fmt::streamed(console_desc));
        return 0;
    }

    const auto lsdPath = std::filesystem::u8path(lsdPathStr);
    const auto lsaPath = std::filesystem::u8path(lsaPathStr);
    const auto dudenPath = std::filesystem::u8path(dudenPathStr);
    const auto bofPath = std::filesystem::u8path(bofPathStr);
    const auto textPath = std::filesystem::u8path(textPathStr);
    const auto hicPath = std::filesystem::u8path(hicPathStr);
    const auto fsiPath = std::filesystem::u8path(fsiPathStr);
    const auto adpPath = std::filesystem::u8path(adpPathStr);
    auto outputPath = std::filesystem::u8path(outputPathStr);
    const auto idxPath = std::filesystem::u8path(idxPathStr);

    ConsoleLog log(verbose);

    try {
        if (!outputPath.empty()) {
            std::filesystem::create_directories(outputPath);
            outputPath = std::filesystem::canonical(outputPath);
        }
        if (!lsdPath.empty()) {
            parseLSD(lsdPath,
                     outputPath,
                     sourceFilter,
                     targetFilter,
                     isDumb,
                     log);
        }
        if (!lsaPath.empty()) {
            decodeLSA(lsaPath, outputPath, log);
        }
#ifdef ENABLE_DUDEN
        if (!dudenPath.empty()) {
            if (dudenPrintInfo) {
                return printDudenInfo(dudenPath, log);
            }
            parseDuden(dudenPath, outputPath, log);
        }
        if (!bofPath.empty() && !idxPath.empty()) {
            decodeBofIdx(bofPath, idxPath, fsiPath, dudenEncoding, outputPath);
        }
        if (!hicPath.empty()) {
            decodeHic(hicPath, outputPath);
        }
        if (!textPath.empty()) {
            parseDudenText(textPath, outputPath, dudenEncoding);
        }
        if (!fsiPath.empty()) {
            parseFsi(fsiPath, outputPath);
        }
        if (!adpPath.empty()) {
            decodeAdp(adpPath, outputPath);
        }
#endif
    } catch (std::exception& exc) {
        fmt::print("an error occurred while processing dictionary: {}\n", exc.what());
        return 1;
    }

    return 0;
}

#ifdef WIN32
int wmain(int argc, wchar_t* wargv[]) {
    std::vector<std::string> u8args;
    for (int i = 0; i < argc; ++i) {
        u8args.push_back(toUtf8(reinterpret_cast<char16_t*>(wargv[i])));
    }
    std::vector<char*> argv;
    for (auto& arg : u8args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    return lsd2dsl_main(argc, &argv[0]);
}
#else
int main(int argc, char* argv[]) {
    return lsd2dsl_main(argc, argv);
}
#endif
