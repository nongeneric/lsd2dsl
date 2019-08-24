#include "lib/common/DslWriter.h"
#include "lib/common/Log.h"
#include "lib/common/ZipWriter.h"
#include "lib/common/bformat.h"
#include "lib/common/overloaded.h"
#include "lib/common/version.h"
#include "lib/common/WavWriter.h"
#include "lib/common/filesystem.h"
#include "lib/duden/AdpDecoder.h"
#include "lib/duden/Dictionary.h"
#include "lib/duden/Duden.h"
#include "lib/duden/HtmlRenderer.h"
#include "lib/duden/InfFile.h"
#include "lib/duden/Writer.h"
#include "lib/duden/text/Parser.h"
#include "lib/duden/text/Printers.h"
#include "lib/duden/text/TextRun.h"
#include "lib/lsd/LSAReader.h"
#include "lib/lsd/lsd.h"
#include "lib/lsd/tools.h"

#include <QApplication>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <string_view>

namespace po = boost::program_options;
using namespace dictlsd;

const std::string dudenIndexParamName = "duden-index";

struct Entry {
    std::u16string heading;
    std::u16string article;
};

int parseLSD(fs::path lsdPath,
             fs::path outputPath,
             int sourceFilter,
             int targetFilter,
             bool dumb,
             Log& log)
{
    FileStream ras(lsdPath.string());
    BitStreamAdapter bstr(&ras);
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
        writeDSL(&reader, lsdPath.filename().string(), outputPath.string(), dumb, log);
    }

    return 0;
}

int printDudenInfo(fs::path infPath, Log& log) {
    duden::FileSystem fs(infPath.parent_path().string());
    FileStream infStream(infPath.string());

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

int parseDuden(fs::path infPath, fs::path outputPath, std::optional<unsigned> index, Log& log) {
    FileStream infStream(infPath.string());
    duden::FileSystem fs(infPath.parent_path().string());
    auto infs = duden::parseInfFile(&infStream, &fs);
    auto inf = infs[0];

    if (infs.size() > 1) {
        if (!index) {
            log.regular("INF file contains %d dictionaries, but no index was specified via --%s",
                        infs.size(),
                        dudenIndexParamName);
            return 1;
        }
        if (*index >= infs.size()) {
            log.regular("INF file contains %d dictionaries, the specified index %d is too large "
                        "(index starts from zero)",
                        infs.size(),
                        *index);
            return 1;
        }
        inf = infs[*index];
    }

    log.regular("Version:  %x", inf.version);

    if (!inf.supported) {
        log.regular("Unsupported dictionary version");
        return 1;
    }

    if (!outputPath.empty()) {
        duden::writeDSL(infPath, outputPath, index ? *index : 0, log);
    }

    return 0;
}

void decodeBofIdx(std::string bofPath,
                  std::string idxPath,
                  std::string fsiPath,
                  bool dudenEncoding,
                  std::string output) {
    FileStream fBof(bofPath);
    FileStream fIdx(idxPath);
    duden::Archive archive(&fIdx, &fBof);
    std::vector<char> vec;

    if (fsiPath.empty()) {
        archive.read(0, -1, vec);
        std::ofstream file((fs::path(output) / "decoded.dump").string());
        if (dudenEncoding) {
            auto text = duden::dudenToUtf8(std::string{begin(vec), end(vec)});
            file.write(&text[0], text.size());
        } else {
            file.write(&vec[0], vec.size());
        }
    } else {
        FileStream fFsi(fsiPath);
        auto entries = duden::parseFsiFile(&fFsi);
        for (auto& entry : entries) {
            UnicodePathFile file((fs::path(output) / entry.name).string(), true);
            std::vector<char> vec;
            archive.read(entry.offset, entry.size, vec);
            file.write(&vec[0], vec.size());
        }
    }
}

void decodeHic(std::string hicPath,
               std::string output) {
    FileStream fHic(hicPath);
    auto hic = duden::parseHicFile(&fHic);
    std::ofstream f((fs::path(output) / "hic.dump").string());
    int leafNum = 0;
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
}

void parseDudenText(std::string textPath, std::string output) {
    duden::ParsingContext context;
    std::ifstream f(textPath);
    if (!f.is_open())
        throw std::runtime_error("can't open file");
    f.seekg(0, std::ios_base::end);
    std::string text(f.tellg(), ' ');
    f.seekg(0);
    f.read(&text[0], text.size());
    auto run = duden::parseDudenText(context, text);
    std::ofstream of(output + "/textdump");
    auto tree = duden::printTree(run);
    of << tree << "\n\n";
    auto dsl = duden::printDsl(run);
    of << dsl << "\n\n";
    auto html = duden::printHtml(run);
    of << html << "\n\n";
}

void parseFsi(std::string fsiPath, std::string output) {
    FileStream fFsi(fsiPath);
    auto entries = duden::parseFsiFile(&fFsi);
    std::ofstream file((fs::path(output) / "fsi.dump").string());
    for (auto& entry : entries) {
        file << bformat("%08x  %08x  %s\n", entry.offset, entry.size, entry.name);
    }
}

void decodeAdp(std::string adpPath, fs::path outputPath) {
    std::ifstream input(adpPath);
    std::ofstream output((outputPath / "decoded.wav").string());
    input.seekg(0, std::ios_base::end);
    std::vector<char> inputVec(input.tellg());
    input.seekg(0);
    input.read(&inputVec[0], inputVec.size());
    std::vector<int16_t> pcmVec(2 * inputVec.size());
    duden::decodeAdp(inputVec, &pcmVec[0]);
    std::vector<char> wav;
    dictlsd::createWav(pcmVec, wav, duden::ADP_SAMPLE_RATE);
    output.write(reinterpret_cast<char*>(&wav[0]), wav.size());
}

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
                std::cout << std::endl;
            }
            std::cout << line << std::endl;
            _lastProgress = false;
        }
    }

    void reportProgress(int percentage) override {
        int left = static_cast<int>(percentage / 100.f * _bar.size());
        int right = _bar.size() - left;
        std::cout << bformat("\r%s %3d%% [%s%s]",
                             _progressName,
                             percentage,
                             std::string_view(_bar).substr(0, left),
                             std::string_view(_empty).substr(0, right));
        if (percentage == 100) {
            std::cout << std::endl;
            _lastProgress = false;
        } else {
            std::cout.flush();
            _lastProgress = true;
        }
    }

    void reportProgressReset(std::string name) override {
        if (_lastProgress) {
            std::cout << std::endl;
        }
        _progressName = name;
        _lastProgress = false;
    }

public:
    ConsoleLog(bool verbose) : _verbose(verbose), _bar(50, '='), _empty(_bar.size(), ' ') {}
};

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    std::string lsdPath, lsaPath, dudenPath, outputPath;
    std::string bofPath, idxPath, fsiPath, hicPath, adpPath, textPath;
    int sourceFilter = -1, targetFilter = -1;
    bool isDumb, dudenEncoding, dudenPrintInfo, verbose;
    int dudenIndex = -1;
    po::options_description console_desc("Allowed options");
    try {
        console_desc.add_options()
            ("help", "produce help message")
            ("lsd", po::value<std::string>(&lsdPath), "LSD dictionary to decode")
            ("duden", po::value<std::string>(&dudenPath), "Duden dictionary to decode (.inf file)")
            (dudenIndexParamName.c_str(), po::value<int>(&dudenIndex), "Index for two-way Duden dictionaries")
            ("lsa", po::value<std::string>(&lsaPath), "LSA sound archive to decode")
            ("source-filter", po::value<int>(&sourceFilter),
                "ignore dictionaries with source language != source-filter")
            ("target-filter", po::value<int>(&targetFilter),
                "ignore dictionaries with target language != target-filter")
            ("codes", "print supported languages and their codes")
            ("out", po::value<std::string>(&outputPath), "output directory")
            ("dumb", "don't combine variant headings and headings "
                     "referencing the same article")
            ("verbose", "verbose logging")
            ("version", "print version")
            ;

        if (g_debug) {
            console_desc.add_options()
                ("bof", po::value<std::string>(&bofPath), "Duden BOF file path")
                ("idx", po::value<std::string>(&idxPath), "Duden IDX file path")
                ("fsi", po::value<std::string>(&fsiPath), "Duden FSI file path")
                ("hic", po::value<std::string>(&hicPath), "Duden HIC file path")
                ("adp", po::value<std::string>(&adpPath), "Duden ADP file path")
                ("text", po::value<std::string>(&textPath), "Duden decoded text file path")
                ("duden-info", "Print dictionary info and return")
                ("duden-utf", "Decode Duden to Utf");
        }

        po::variables_map console_vm;
        po::store(po::parse_command_line(argc, argv, console_desc), console_vm);
        if (console_vm.count("help")) {
            std::cout << console_desc;
            return 0;
        }
        if (console_vm.count("codes")) {
            printLanguages(std::cout);
            return 0;
        }
        if (console_vm.count("version")) {
            std::cout << "lsd2dsl version " << g_version << std::endl;
            return 0;
        }
        isDumb = console_vm.count("dumb");
        verbose = console_vm.count("verbose");
        dudenEncoding = console_vm.count("duden-utf");
        dudenPrintInfo = console_vm.count("duden-info");
        po::notify(console_vm);
    } catch(std::exception& e) {
        std::cout << "can't parse program options:\n";
        std::cout << e.what() << "\n\n";
        std::cout << console_desc;
        return 1;
    }

    if (lsdPath.empty() && lsaPath.empty() && dudenPath.empty() && bofPath.empty() &&
        textPath.empty() && hicPath.empty() && fsiPath.empty() && adpPath.empty()) {
        std::cout << console_desc;
        return 0;
    }

    ConsoleLog log(verbose);

    try {
        if (!outputPath.empty()) {
            fs::create_directories(outputPath);
            outputPath = fs::canonical(outputPath).string();
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
        if (!dudenPath.empty()) {
            if (dudenPrintInfo) {
                return printDudenInfo(dudenPath, log);
            }
            std::optional<unsigned> optional;
            if (dudenIndex != -1) {
                optional = static_cast<unsigned>(dudenIndex);
            }
            parseDuden(dudenPath, outputPath, optional, log);
        }
        if (!bofPath.empty() && !idxPath.empty()) {
            decodeBofIdx(bofPath, idxPath, fsiPath, dudenEncoding, outputPath);
        }
        if (!hicPath.empty()) {
            decodeHic(hicPath, outputPath);
        }
        if (!textPath.empty()) {
            parseDudenText(textPath, outputPath);
        }
        if (!fsiPath.empty()) {
            parseFsi(fsiPath, outputPath);
        }
        if (!adpPath.empty()) {
            decodeAdp(adpPath, outputPath);
        }
    } catch (std::exception& exc) {
        std::cout << "an error occured while processing dictionary: " << exc.what() << std::endl;
        return 1;
    }

    return 0;
}
