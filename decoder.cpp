#include "ZipWriter.h"

#include "dictlsd/lsd.h"
#include "dictlsd/BitStream.h"
#include "dictlsd/tools.h"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

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

unsigned getFileSize(std::fstream& file) {
    unsigned pos = file.tellg();
    file.seekg(0, std::ios_base::end);
    unsigned size = file.tellg();
    file.seekg(pos, std::ios_base::beg);
    return size;
}

void normalizeArticle(std::u16string& str) {
    for (int i = str.length() - 1; i >= 0; --i) {
        if (str[i] == u'\n') {
            str.insert(i + 1, 1, u'\t');
        }
    }
}

template <typename T>
std::vector<T>& append(std::vector<T>& vec, std::vector<T> const& rhs) {
    std::copy(rhs.begin(), rhs.end(), std::back_inserter(vec));
    return vec;
}

int parseLSD(fs::path lsdPath,
             fs::path dslPath,
             fs::path annoPath,
             fs::path iconPath,
             fs::path overlayPath,
             int sourceFilter,
             int targetFilter,
             std::ostream& log)
{
    std::fstream lsd(lsdPath.string(), std::ios::in | std::ios::binary);
    if (!lsd.is_open())
        throw std::runtime_error("Can't open the LSD file.");

    unsigned fileSize = getFileSize(lsd);
    std::unique_ptr<char> buf(new char[fileSize]);
    lsd.read(buf.get(), fileSize);

    InMemoryStream ras(buf.get(), fileSize);
    BitStreamAdapter bstr(&ras);
    LSDDictionary reader(&bstr);
    LSDHeader header = reader.header();
    log << "Header:";    
    log << "\n  Version:  " << std::hex << header.version << std::dec;
    log << "\n  Entries:  " << header.entriesCount;
    log << "\n  Source:   " << header.sourceLanguage
        << " (" << toUtf8(langFromCode(header.sourceLanguage)) << ")";
    log << "\n  Target:   " << header.targetLanguage
        << " (" << toUtf8(langFromCode(header.targetLanguage)) << ")";
    log << "\n  Name:     " << toUtf8(reader.name()) << std::endl;

    if ((sourceFilter != -1 && sourceFilter != header.sourceLanguage) ||
        (targetFilter != -1 && targetFilter != header.targetLanguage))
    {
        log << "ignoring\n";
        return 0;
    }

    auto overlayHeadings = reader.readOverlayHeadings();
    if (overlayHeadings.size() > 0) {
        log << "decoding overlay (" << overlayHeadings.size() << " entries): " <<
               overlayPath << std::endl;
        ZipWriter zip(overlayPath.string());
        for (OverlayHeading heading : overlayHeadings) {
            std::vector<uint8_t> entry = reader.readOverlayEntry(heading);
            zip.addFile(toUtf8(heading.name), entry.data(), entry.size());
        }
    }

    const char16_t* bom = u"\ufeff";

    std::u16string annoStr = reader.annotation();
    if (!annoStr.empty()) {
        log << "writing annotation.. " << annoPath << std::endl;

        std::fstream anno(annoPath.string(), std::ios_base::binary | std::ios_base::out);
        if (!anno.is_open()) {
            throw std::runtime_error("can't create the annotation file");
        }
        anno.write((char*)bom, 2);
        anno.write((char*)annoStr.c_str(), 2 * annoStr.length());
    }

    auto iconArr = reader.icon();
    if (!iconArr.empty()) {
        log << "writing icon.. " << iconPath << std::endl;

        std::fstream icon(iconPath.string(), std::ios_base::binary | std::ios_base::out);
        if (!icon.is_open()) {
            throw std::runtime_error("can't create the icon file");
        }
        icon.write(&iconArr[0], iconArr.size());
    }

    log << "decoding dictionary..\n";

    typedef std::vector<std::tuple<std::u16string, std::u16string>> dvec;
    dvec dict;
    auto headings = reader.readHeadings();
    if (headings.size() != header.entriesCount) {
        throw std::runtime_error("decoding error");
    }

    for (auto heading : headings) {
        dict.push_back(make_tuple(heading.extText(), reader.readArticle(heading.articleReference())));
    }

    log << "writing dsl.. " << dslPath << "\n";

    std::fstream dsl(dslPath.string(), std::ios_base::binary | std::ios_base::out);
    if (!dsl.is_open()) {
        throw std::runtime_error("can't create the dsl file");
    }

    dsl.write((char*)bom, 2);

    auto dslwrite = [&](std::u16string const& line) {
        dsl.write((char*)line.c_str(), 2 * line.length());
    };

    dslwrite(u"#NAME\t\"" + reader.name() + u"\"\r\n");
    dslwrite(u"#INDEX_LANGUAGE\t\"" + langFromCode(header.sourceLanguage) + u"\"\n");
    dslwrite(u"#CONTENTS_LANGUAGE\t\"" + langFromCode(header.targetLanguage) + u"\"\n");
    if (!iconArr.empty()) {
        dslwrite(u"#ICON_FILE\t\"" + toUtf16(iconPath.filename().string()) + u"\"\n");
    }
    dslwrite(u"\n");
    for (auto& pair : dict) {
        const std::u16string& heading = std::get<0>(pair);
        std::u16string article = std::get<1>(pair);
        normalizeArticle(article);
        dslwrite(heading.c_str());
        dslwrite(u"\n\t");
        dslwrite(article.c_str());
        dslwrite(u"\n");
    }

    return 0;
}

int main(int argc, char* argv[]) {
    std::string lsdPath, outputPath;
    int sourceFilter = -1, targetFilter = -1;
    po::options_description console_desc("Allowed options");
    try {
        console_desc.add_options()
            ("help", "produce help message")
            ("lsd", po::value<std::string>(&lsdPath), "LSD dictionary to decode")
            ("source-filter", po::value<int>(&sourceFilter),
                "ignore dictionaries with source language != source-filter")
            ("target-filter", po::value<int>(&targetFilter),
                "ignore dictionaries with target language != target-filter")
            ("codes", "print supported languages and their codes")
            ("out", po::value<std::string>(&outputPath)->required(), "output directory")
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
        po::notify(console_vm);
    } catch(std::exception& e) {
        std::cout << "can't parse program options:\n";
        std::cout << e.what() << "\n\n";
        std::cout << console_desc;
        return 1;
    }

    try {
        fs::create_directories(outputPath);
        if (!lsdPath.empty()) {
            fs::path dslPath = outputPath / fs::path(lsdPath).filename().replace_extension("dsl");
            fs::path annoPath = dslPath;
            fs::path iconPath = dslPath;
            fs::path overlayPath = fs::path(dslPath).string() + ".files.zip";
            annoPath.replace_extension("ann");
            iconPath.replace_extension("bmp");
            parseLSD(lsdPath,
                     dslPath,
                     annoPath,
                     iconPath,
                     overlayPath,
                     sourceFilter,
                     targetFilter,
                     std::cout);
        }
    } catch (std::exception& exc) {
        std::cout << "an error occured while processing dictionary: " << exc.what() << std::endl;
        return 1;
    }

    return 0;
}
