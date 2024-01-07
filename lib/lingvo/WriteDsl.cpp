#include "WriteDsl.h"

#include "common/DslWriter.h"
#include "common/ZipWriter.h"
#include "tools.h"

namespace lingvo {

void writeDSL(const LSDDictionary* reader,
              std::filesystem::path lsdName,
              std::filesystem::path outputPath,
              bool dumb,
              Log& log)
{
    dsl::Writer writer(outputPath, lsdName.replace_extension().u8string());
    std::filesystem::path overlayPath = std::filesystem::u8path(writer.dslFilePath().u8string() + ".files.zip");

    auto overlayHeadings = reader->readOverlayHeadings();
    if (overlayHeadings.size() > 0) {
        log.resetProgress("overlay", overlayHeadings.size());
        ZipWriter zip(overlayPath);
        for (OverlayHeading heading : overlayHeadings) {
            std::vector<uint8_t> entry = reader->readOverlayEntry(heading);
            zip.addFile(toUtf8(heading.name), entry.data(), entry.size());
            log.advance();
        }
    }

    std::u16string annoStr = reader->annotation();
    if (!annoStr.empty()) {
        writer.setAnnotation(annoStr);
    }

    auto headings = reader->readHeadings();
    if (headings.size() != reader->header().entriesCount) {
        throw std::runtime_error("decoding error");
    }

    if (!dumb) {
        log.regular("collapsing variant headings");
        collapseVariants(headings);
    }

    writer.setName(reader->name());
    writer.setLanguage(reader->header().sourceLanguage, reader->header().targetLanguage);
    auto iconArr = reader->icon();
    if (!iconArr.empty()) {
        writer.setIcon(iconArr);
    }
    writer.writeNewLine();

    log.resetProgress(writer.dslFileName().u8string(), headings.size());
    foreachReferenceSet(headings, [&](auto first, auto last) {
        for (auto it = first; it != last; ++it) {
            const std::u16string& headingText = it->dslText();
            writer.writeHeading(headingText);
            log.advance();
        }
        std::u16string article = reader->readArticle(first->articleReference());
        writer.writeArticle(article);
    }, dumb);
}

}
