#include "DslWriter.h"
#include "ZipWriter.h"
#include "dictlsd/tools.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <fstream>

using namespace dictlsd;
namespace fs = boost::filesystem;

void normalizeArticle(std::u16string& str) {
    for (int i = str.length() - 1; i >= 0; --i) {
        if (str[i] == u'\n') {
            str.insert(i + 1, 1, u'\t');
        }
    }
}

void writeDSL(const LSDDictionary* reader,
              std::string lsdName,
              std::string outputPath,
              bool dumb,
              std::function<void(int,std::string)> log)
{
    fs::path dslPath = outputPath / fs::path(lsdName).replace_extension("dsl");
    fs::path annoPath = dslPath;
    fs::path iconPath = dslPath;
    fs::path overlayPath = fs::path(dslPath).string() + ".files.zip";
    annoPath.replace_extension("ann");
    iconPath.replace_extension("bmp");
    auto overlayHeadings = reader->readOverlayHeadings();
    if (overlayHeadings.size() > 0) {
        log(5, str(boost::format("decoding overlay (%1% entries): %2%") % overlayHeadings.size() % overlayPath.string()));
        ZipWriter zip(overlayPath.string());
        for (OverlayHeading heading : overlayHeadings) {
            std::vector<uint8_t> entry = reader->readOverlayEntry(heading);
            zip.addFile(toUtf8(heading.name), entry.data(), entry.size());
        }
    }

    const char16_t* bom = u"\ufeff";

    std::u16string annoStr = reader->annotation();
    if (!annoStr.empty()) {
        log(25, "writing annotation: " + annoPath.string());
        std::fstream anno(annoPath.string(), std::ios_base::binary | std::ios_base::out);
        if (!anno.is_open()) {
            throw std::runtime_error("can't create the annotation file");
        }
        anno.write((char*)bom, 2);
        anno.write((char*)annoStr.c_str(), 2 * annoStr.length());
    }

    auto iconArr = reader->icon();
    if (!iconArr.empty()) {
        log(35, "writing icon: " + iconPath.string());
        std::fstream icon(iconPath.string(), std::ios_base::binary | std::ios_base::out);
        if (!icon.is_open()) {
            throw std::runtime_error("can't create the icon file");
        }
        icon.write(reinterpret_cast<const char*>(&iconArr[0]), iconArr.size());
    }

    log(45, "decoding dictionary");

    auto headings = reader->readHeadings();
    if (headings.size() != reader->header().entriesCount) {
        throw std::runtime_error("decoding error");
    }

    if (!dumb) {
        log(60, "collapsing variant headings");
        collapseVariants(headings);
    }

    log(80, "writing dsl: " + dslPath.string());
    std::fstream dsl(dslPath.string(), std::ios_base::binary | std::ios_base::out);
    if (!dsl.is_open()) {
        throw std::runtime_error("can't create the dsl file");
    }

    dsl.write((char*)bom, 2);

    auto dslwrite = [&](std::u16string const& line) {
        dsl.write((char*)line.c_str(), 2 * line.length());
    };

    dslwrite(u"#NAME\t\"" + reader->name() + u"\"\r\n");
    dslwrite(u"#INDEX_LANGUAGE\t\"" + langFromCode(reader->header().sourceLanguage) + u"\"\n");
    dslwrite(u"#CONTENTS_LANGUAGE\t\"" + langFromCode(reader->header().targetLanguage) + u"\"\n");
    if (!iconArr.empty()) {
        dslwrite(u"#ICON_FILE\t\"" + toUtf16(iconPath.filename().string()) + u"\"\n");
    }
    dslwrite(u"\n");
    foreachReferenceSet(headings, [&](auto first, auto last) {
        for (auto it = first; it != last; ++it) {
            const std::u16string& headingText = it->extText();
            dslwrite(headingText.c_str());
            dslwrite(u"\n");
        }
        dslwrite(u"\t");
        std::u16string article = reader->readArticle(first->articleReference());
        normalizeArticle(article);
        dslwrite(article.c_str());
        dslwrite(u"\n");
    }, dumb);
}
