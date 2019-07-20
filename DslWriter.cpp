#include "DslWriter.h"
#include "ZipWriter.h"
#include "dictlsd/tools.h"
#include "tools/bformat.h"
#include <boost/filesystem.hpp>

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
    dsl::Writer writer(outputPath, lsdName);
    fs::path overlayPath = fs::path(writer.dslFileName()).string() + ".files.zip";

    auto overlayHeadings = reader->readOverlayHeadings();
    if (overlayHeadings.size() > 0) {
        log(5, bformat("decoding overlay (%1% entries): %2%", overlayHeadings.size(), overlayPath.string()));
        ZipWriter zip(overlayPath.string());
        for (OverlayHeading heading : overlayHeadings) {
            std::vector<uint8_t> entry = reader->readOverlayEntry(heading);
            zip.addFile(toUtf8(heading.name), entry.data(), entry.size());
        }
    }

    std::u16string annoStr = reader->annotation();
    if (!annoStr.empty()) {
        writer.setAnnotation(annoStr);
    }

    auto iconArr = reader->icon();
    if (!iconArr.empty()) {
        writer.setIcon(iconArr);
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

    log(80, "writing dsl: " + writer.dslFileName());

    writer.setName(reader->name());
    writer.setLanguage(reader->header().sourceLanguage, reader->header().targetLanguage);

    foreachReferenceSet(headings, [&](auto first, auto last) {
        for (auto it = first; it != last; ++it) {
            const std::u16string& headingText = it->dslText();
            writer.writeHeading(headingText);
        }
        std::u16string article = reader->readArticle(first->articleReference());
        writer.writeArticle(article);
    }, dumb);
}

namespace dsl {

    constexpr char _utf16bom[] { (char)0xff, (char)0xfe };

    void Writer::write(const std::u16string& line) {
        _dsl->write((char*)line.c_str(), 2 * line.length());
    }

    Writer::Writer(std::string outputPath, std::string name) {
        _dslPath = outputPath / fs::path(name).replace_extension("dsl");
        _dsl.reset(new UnicodePathFile(_dslPath.string(), true));
        _dsl->write(_utf16bom, sizeof(_utf16bom));
    }

    std::string Writer::dslFileName() const {
        return _dslPath.filename().string();
    }

    void Writer::setName(std::u16string name) {
        write(u"#NAME\t\"" + name + u"\"\r\n");
    }

    void Writer::setAnnotation(std::u16string annotation) {
        auto path = _dslPath;
        path.replace_extension("ann");
        UnicodePathFile anno(path.string(), true);
        anno.write(_utf16bom, sizeof(_utf16bom));
        anno.write((char*)annotation.c_str(), 2 * annotation.length());
    }

    void Writer::setLanguage(int source, int target) {
        write(u"#INDEX_LANGUAGE\t\"" + langFromCode(source) + u"\"\n");
        write(u"#CONTENTS_LANGUAGE\t\"" + langFromCode(target) + u"\"\n");
    }

    void Writer::setIcon(std::vector<uint8_t> icon) {
        auto path = _dslPath;
        path.replace_extension("bmp");

        write(u"#ICON_FILE\t\"" + toUtf16(path.filename().string()) + u"\"\n");
        write(u"\n");

        UnicodePathFile file(path.string(), true);
        file.write(reinterpret_cast<const char*>(&icon[0]), icon.size());
    }

    void Writer::writeHeading(std::u16string heading) {
        write(heading.c_str());
        write(u"\n");
    }

    void Writer::writeArticle(std::u16string article) {
        write(u"\t");
        normalizeArticle(article);
        write(article.c_str());
        write(u"\n");
    }

} // namespace dsl
