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

void writeDSL(fs::path inputDir,
              Dictionary& dict,
              InfFile const& inf,
              fs::path outputPath,
              ProgressCallback progress) {
    std::string dslFileName = "dictionary";

    auto entries = dict.entries();
    entries.erase(std::remove_if(begin(entries), end(entries), [](auto& entry) {
        return entry.type != duden::HicEntryType::Plain;
    }), end(entries));

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

    for (auto i = 0u; i < entries.size(); ++i) {
        auto size = i == entries.size() - 1 ? -1u
                  : entries[i + 1].textOffset - entries[i].textOffset;
        auto article = dict.article(entries[i].textOffset, size);
        ParsingContext context;
        auto headingRun = parseDudenText(context, entries[i].heading);
        auto articleRun = parseDudenText(context, article);
        resolveReferences(context, articleRun, dict.ld());
        inlineReferences(context, articleRun, resources);
        resolveReferences(context, articleRun, dict.ld());
        tableRenderer.render(articleRun);
        dedupHeading(headingRun, articleRun);
        auto dslArticle = printDsl(articleRun);
        auto dslHeading = printDsl(headingRun);

        writer.writeHeading(dictlsd::toUtf16(dslHeading));
        writer.writeArticle(dictlsd::toUtf16(dslArticle));
    }
}

} // namespace duden
