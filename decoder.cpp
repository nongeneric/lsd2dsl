#include "version.h"
#include "ZipWriter.h"
#include "DslWriter.h"
#include "dictlsd/lsd.h"
#include "dictlsd/tools.h"
#include "dictlsd/LSAReader.h"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <iostream>
#include <fstream>
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

template <typename T>
std::vector<T>& append(std::vector<T>& vec, std::vector<T> const& rhs) {
    std::copy(rhs.begin(), rhs.end(), std::back_inserter(vec));
    return vec;
}

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

int main(int argc, char* argv[]) {
    std::string lsdPath, lsaPath, outputPath;
    int sourceFilter = -1, targetFilter = -1;
    bool isDumb;
    po::options_description console_desc("Allowed options");
    try {
        console_desc.add_options()
            ("help", "produce help message")
            ("lsd", po::value<std::string>(&lsdPath), "LSD dictionary to decode")
            ("lsa", po::value<std::string>(&lsaPath), "LSA sound archive to decode")
            ("source-filter", po::value<int>(&sourceFilter),
                "ignore dictionaries with source language != source-filter")
            ("target-filter", po::value<int>(&targetFilter),
                "ignore dictionaries with target language != target-filter")
            ("codes", "print supported languages and their codes")
            ("out", po::value<std::string>(&outputPath), "output directory")
            ("dumb", "don't combine variant headings and headings "
                     "referencing the same article")
            ("version", "print version")
            ;
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
        po::notify(console_vm);
    } catch(std::exception& e) {
        std::cout << "can't parse program options:\n";
        std::cout << e.what() << "\n\n";
        std::cout << console_desc;
        return 1;
    }

    if (lsdPath.empty() && lsaPath.empty()) {
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
    } catch (std::exception& exc) {
        std::cout << "an error occured while processing dictionary: " << exc.what() << std::endl;
        return 1;
    }

    return 0;
}
