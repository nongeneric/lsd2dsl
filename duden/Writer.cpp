#include "Writer.h"
#include "DslWriter.h"
#include "ZipWriter.h"
#include "TableRenderer.h"
#include "tools/bformat.h"
#include "duden/Duden.h"
#include "duden/Archive.h"
#include "duden/text/TextRun.h"
#include "duden/text/Parser.h"
#include "duden/text/Printers.h"
#include "duden/text/Reference.h"
#include "dictlsd/tools.h"

namespace duden {

using namespace std::literals;

void writeDSL(fs::path inputDir,
              Dictionary& dict,
              InfFile const& inf,
              fs::path outputPath,
              ProgressCallback progress) {
    std::string dslFileName = "dictionary";

    auto entries = dict.entries();
    auto groups = groupHicEntries(entries);

    progress(0, LogLevel::Regular, bformat("found %d articles", groups.size()));

    auto overlayPath = outputPath / (dslFileName + ".dsl.files.zip");
    ZipWriter zip(overlayPath.string());

    ResourceFiles resources;
    for (auto& pack : inf.resources) {
        if (!pack.fsi.empty())
            continue;

        progress(
            0,
            LogLevel::Regular,
            bformat("unpacking %s (using index %s)", pack.bof, pack.idx));

        boost::filesystem::path inputPath = inputDir;
        dictlsd::FileStream fIndex((inputPath / pack.idx).string());
        dictlsd::FileStream fBof((inputPath / pack.bof).string());
        Archive archive(&fIndex, &fBof);
        std::vector<char> vec;
        archive.read(0, -1, vec);
        auto stream = std::make_unique<std::stringstream>();
        stream->write(vec.data(), vec.size());
        stream->seekg(0);
        auto filename = boost::filesystem::path(pack.bof).stem().string();
        resources[filename] = std::move(stream);
    }

    dsl::Writer writer(outputPath.string(), dslFileName);
    writer.setName(dictlsd::toUtf16(inf.name));

    std::map<std::string, std::tuple<const ResourceArchive*, uint32_t, uint32_t>> resourceIndex;

    for (auto& pack : inf.resources) {
        if (pack.fsi.empty())
            continue;

        progress(
            0,
            LogLevel::Regular,
            bformat(
                "unpacking resources from %s (with index %s and names from %s)",
                pack.bof,
                pack.idx,
                pack.fsi));

        boost::filesystem::path inputPath = inputDir;
        dictlsd::FileStream fFsi((inputPath / pack.fsi).string());
        dictlsd::FileStream fIndex((inputPath / pack.idx).string());
        dictlsd::FileStream fBof((inputPath / pack.bof).string());
        auto entries = parseFsiFile(&fFsi);

        progress(0, LogLevel::Regular, bformat("found %d entries", entries.size()));

        Archive archive(&fIndex, &fBof);
        std::vector<char> vec;
        int i = 0;
        for (auto& entry : entries) {
            resourceIndex[entry.name] = {&pack, entry.offset, entry.size};
            progress(
                0,
                LogLevel::Verbose,
                bformat("unpacking [%03d/%03d] %s", i, entries.size(), entry.name));
            archive.read(entry.offset, entry.size, vec);
            zip.addFile(entry.name, vec.data(), vec.size());
            ++i;
        }
    }

    progress(0, LogLevel::Regular, "processing articles");

    TableRenderer tableRenderer([&](auto span, auto name) {
        progress(0, LogLevel::Verbose, bformat("created %s", name));
        zip.addFile(name, span.data(), span.size());
    }, [&](auto name) {
        progress(0, LogLevel::Verbose, bformat("table has embedded image %s", name));
        auto it = resourceIndex.find(name);
        std::vector<char> vec;
        if (it == end(resourceIndex)) {
            progress(0, LogLevel::Regular, bformat("embedded image %s doesn't exist", name));
            return vec;
        }
        auto [pack, offset, size] = it->second;
        boost::filesystem::path inputPath = inputDir;
        dictlsd::FileStream fIndex((inputPath / pack->idx).string());
        dictlsd::FileStream fBof((inputPath / pack->bof).string());
        Archive archive(&fIndex, &fBof);
        archive.read(offset, size, vec);
        return vec;
    });

    for (const auto& [textOffset, group] : groups) {
        // headings of pictures, tables, etc
        if (static_cast<unsigned>(textOffset) >= dict.articleArchiveDecodedSize())
            continue;
        auto article = dict.article(textOffset, group.articleSize);
        ParsingContext context;
        TextRun* headingRun = nullptr;
        for (const auto& heading : group.headings) {
            headingRun = parseDudenText(context, heading);
            auto dslHeading = printDsl(headingRun);
            writer.writeHeading(dictlsd::toUtf16(dslHeading));
        }
        auto articleRun = parseDudenText(context, article);
        resolveReferences(context, articleRun, dict.ld());
        inlineReferences(context, articleRun, resources);
        resolveReferences(context, articleRun, dict.ld());
        const auto& firstHeading = group.headings.front();
        resolveArticleReferences(articleRun, [&](auto offset) {
            auto it = groups.find(offset - 1);
            if (it == end(groups)) {
                progress(0,
                         LogLevel::Regular,
                         bformat("Article [%s] references unknown article %d", firstHeading, offset - 1));
                return "unknown"s;
            }
            return it->second.headings.front();
        });
        tableRenderer.render(articleRun);
        if (group.headings.size() == 1) {
            dedupHeading(headingRun, articleRun);
        }
        auto dslArticle = printDsl(articleRun);
        writer.writeArticle(dictlsd::toUtf16(dslArticle));
    }
}

} // namespace duden
