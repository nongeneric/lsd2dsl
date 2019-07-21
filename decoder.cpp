#include "version.h"
#include "ZipWriter.h"
#include "DslWriter.h"
#include "dictlsd/lsd.h"
#include "dictlsd/tools.h"
#include "tools/bformat.h"
#include "dictlsd/LSAReader.h"
#include "duden/Duden.h"
#include "duden/InfFile.h"
#include "duden/Dictionary.h"
#include "duden/Writer.h"
#include "duden/text/TextRun.h"
#include "duden/text/Parser.h"
#include "duden/text/Printers.h"
#include "duden/HtmlRenderer.h"

#include <QtGui/QApplication>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <map>

namespace fs = boost::filesystem;
namespace po = boost::program_options;
using namespace dictlsd;

struct Entry {
    std::u16string heading;
    std::u16string article;
};

int parseLSD(fs::path lsdPath,
             fs::path outputPath,
             int sourceFilter,
             int targetFilter,
             bool dumb,
             std::ostream& log)
{
    FileStream ras(lsdPath.string());
    BitStreamAdapter bstr(&ras);
    LSDDictionary reader(&bstr);
    LSDHeader header = reader.header();
    log << "Header:";
    log << "\n  Version:  " << std::hex << header.version << std::dec;
    log << "\n  Headings: " << header.entriesCount;
    log << "\n  Source:   " << header.sourceLanguage
        << " (" << toUtf8(langFromCode(header.sourceLanguage)) << ")";
    log << "\n  Target:   " << header.targetLanguage
        << " (" << toUtf8(langFromCode(header.targetLanguage)) << ")";
    log << "\n  Name:     " << toUtf8(reader.name()) << std::endl;

    if (!reader.supported()) {
        log << "Unsupported dictionary version\n"
            "Only the following versions are implemented\n"
            "  Lingvo x6: 151005, 152001, 155001\n"
            "  Lingvo x5: 141004, 142001, 145001\n"
            "  Lingvo x3: 131001, 132001\n"
            "  Lingvo 12: 120001\n"
            "  Lingvo 11: 110001\n";
        return 1;
    }

    if ((sourceFilter != -1 && sourceFilter != header.sourceLanguage) ||
        (targetFilter != -1 && targetFilter != header.targetLanguage))
    {
        log << "ignoring\n";
        return 0;
    }

    if (!outputPath.empty()) {
        writeDSL(&reader, lsdPath.filename().string(), outputPath.string(),
                 dumb, [&](int, std::string s) { log << s << std::endl; });
    }

    return 0;
}

int parseDuden(fs::path infPath, fs::path outputPath, bool verbose, std::ostream& log) {
    FileStream infStream(infPath.string());
    auto inf = duden::parseInfFile(&infStream);
    duden::FileSystem fs(infPath.parent_path().string());
    duden::fixFileNameCase(inf, &fs);
    duden::Dictionary dict(&fs, inf);

    log << "Header:";
    log << "\n  Headings: " << dict.articleCount();
    log << "\n  Version:  " << std::hex << inf.version << std::dec;
    log << "\n  Name:     " << inf.name << std::endl;

    if (!inf.supported) {
        log << "Unsupported dictionary version\n";
        return 1;
    }

    auto progress = [&](int i, auto level, auto message) {
        if (level == duden::LogLevel::Verbose && verbose) {
            log << message << std::endl;
        } else if (level == duden::LogLevel::Regular) {
            log << bformat("% 3d %s", i, message) << std::endl;
        }
    };

    if (!outputPath.empty()) {
        duden::writeDSL(infPath.parent_path().string(),
                        dict,
                        inf,
                        outputPath.string(),
                        progress);
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
        UnicodePathFile file((fs::path(output) / "decoded.bin").string(), true);
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

void parseDudenText(std::string textPath, std::string output) {
    duden::ParsingContext context;
    std::ifstream f(textPath);
    if (!f.is_open())
        throw std::runtime_error("can't open file");
    std::istream_iterator<char> eof;
    std::istream_iterator<char> it(f);
    std::string text {it, eof};
    auto run = duden::parseDudenText(context, text);
    std::ofstream of(output + "/textdump");
    auto tree = duden::printTree(run);
    of << tree << "\n\n";
    auto dsl = duden::printDsl(run);
    of << dsl << "\n\n";
    auto html = duden::printHtml(run);
    of << html << "\n\n";
}

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    std::string lsdPath, lsaPath, dudenPath, outputPath;
    std::string bofPath, idxPath, fsiPath, textPath;
    int sourceFilter = -1, targetFilter = -1;
    bool isDumb, dudenEncoding, verbose;
    po::options_description console_desc("Allowed options");
    try {
        console_desc.add_options()
            ("help", "produce help message")
            ("lsd", po::value<std::string>(&lsdPath), "LSD dictionary to decode")
            ("duden", po::value<std::string>(&dudenPath), "Duden dictionary to decode (.inf file)")
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
                ("text", po::value<std::string>(&textPath), "Duden decoded text file path")
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
        po::notify(console_vm);
    } catch(std::exception& e) {
        std::cout << "can't parse program options:\n";
        std::cout << e.what() << "\n\n";
        std::cout << console_desc;
        return 1;
    }

    if (lsdPath.empty() && lsaPath.empty() && dudenPath.empty() && bofPath.empty() &&
        textPath.empty()) {
        std::cout << console_desc;
        return 0;
    }

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
                     std::cout);
        }
        if (!lsaPath.empty()) {
            decodeLSA(lsaPath, outputPath, [](int i) { std::cout << i << std::endl; });
        }
        if (!dudenPath.empty()) {
            parseDuden(dudenPath, outputPath, verbose, std::cout);
        }
        if (!bofPath.empty() && !idxPath.empty()) {
            decodeBofIdx(bofPath, idxPath, fsiPath, dudenEncoding, outputPath);
        }
        if (!textPath.empty()) {
            parseDudenText(textPath, outputPath);
        }
    } catch (std::exception& exc) {
        std::cout << "an error occured while processing dictionary: " << exc.what() << std::endl;
        return 1;
    }

    return 0;
}
