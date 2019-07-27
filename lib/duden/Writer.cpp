#include "Writer.h"
#include "lib/common/DslWriter.h"
#include "lib/common/ZipWriter.h"
#include "TableRenderer.h"
#include "lib/common/bformat.h"
#include "lib/duden/Duden.h"
#include "lib/duden/Archive.h"
#include "lib/duden/text/TextRun.h"
#include "lib/duden/text/Parser.h"
#include "lib/duden/text/Printers.h"
#include "lib/duden/text/Reference.h"
#include "lib/lsd/tools.h"

namespace duden {

using namespace std::literals;

void writeDSL(fs::path infPath,
              fs::path outputPath,
              Log& log) {
    auto inputPath = infPath.parent_path();
    dictlsd::FileStream infStream(infPath.string());
    auto inf = duden::parseInfFile(&infStream);
    duden::FileSystem fs(infPath.parent_path().string());
    duden::fixFileNameCase(inf, &fs);
    duden::Dictionary dict(&fs, inf);

    std::string dslFileName = "dictionary";

    auto entries = dict.entries();
    auto groups = groupHicEntries(entries);

    auto overlayPath = outputPath / (dslFileName + ".dsl.files.zip");
    ZipWriter zip(overlayPath.string());

    ResourceFiles resources;
    for (auto& pack : inf.resources) {
        if (!pack.fsi.empty())
            continue;

        log.regular("unpacking %s", pack.bof);

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

    int resourceCount = 0;

    for (auto& pack : inf.resources) {
        if (pack.fsi.empty())
            continue;

        dictlsd::FileStream fFsi((inputPath / pack.fsi).string());
        dictlsd::FileStream fIndex((inputPath / pack.idx).string());
        dictlsd::FileStream fBof((inputPath / pack.bof).string());
        auto entries = parseFsiFile(&fFsi);

        log.resetProgress(pack.bof, entries.size());

        Archive archive(&fIndex, &fBof);
        std::vector<char> vec;
        int i = 0;
        for (auto& entry : entries) {
            resourceIndex[entry.name] = {&pack, entry.offset, entry.size};
            log.verbose("unpacking [%03d/%03d] %s", i, entries.size(), entry.name);
            archive.read(entry.offset, entry.size, vec);
            zip.addFile(entry.name, vec.data(), vec.size());
            ++i;
            ++resourceCount;
            log.advance();
        }
    }

    log.resetProgress("articles", groups.size());

    int tableCount = 0;

    TableRenderer tableRenderer([&](auto span, auto name) {
        log.verbose("created %s", name);
        ++tableCount;
        zip.addFile(name, span.data(), span.size());
    }, [&](auto name) {
        log.verbose("table has embedded image %s", name);
        auto it = resourceIndex.find(name);
        std::vector<char> vec;
        if (it == end(resourceIndex)) {
            log.regular("embedded image %s doesn't exist", name);
            return vec;
        }
        auto [pack, offset, size] = it->second;
        dictlsd::FileStream fIndex((inputPath / pack->idx).string());
        dictlsd::FileStream fBof((inputPath / pack->bof).string());
        Archive archive(&fIndex, &fBof);
        archive.read(offset, size, vec);
        return vec;
    });

    int articleCount = 0;

    for (const auto& [textOffset, group] : groups) {
        log.advance();
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
                log.regular("Article [%s] references unknown article %d", firstHeading, offset - 1);
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
        ++articleCount;
    }

    log.regular("done converting %d articles, %d tables and %d resources",
                articleCount,
                tableCount,
                resourceCount);
}

} // namespace duden