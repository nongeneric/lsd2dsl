#include "Writer.h"
#include "lib/common/DslWriter.h"
#include "lib/common/ZipWriter.h"
#include "TableRenderer.h"
#include "lib/common/bformat.h"
#include "lib/duden/Duden.h"
#include "lib/duden/Archive.h"
#include "lib/duden/AdpDecoder.h"
#include "lib/duden/FsdFile.h"
#include "lib/duden/text/TextRun.h"
#include "lib/duden/text/Parser.h"
#include "lib/duden/text/Printers.h"
#include "lib/duden/text/Reference.h"
#include "lib/lsd/tools.h"
#include "lib/common/WavWriter.h"

namespace duden {

using namespace std::literals;

namespace {

class ResourceFileSystem : public IFileSystem {
    CaseInsensitiveSet _paths;

public:
    ResourceFileSystem(const std::vector<std::string>& names) {
        for (auto& name : names) {
            _paths.insert(fs::path(name));
        }
    }

    std::unique_ptr<dictlsd::IRandomAccessStream> open(fs::path) override {
        return {};
    }

    const CaseInsensitiveSet& files() override {
        return _paths;
    }
};

uint32_t calcBofOffset(duden::Dictionary& dict) {
    auto head = dict.readEncoded(0, 0x200);
    auto it = std::find(begin(head), end(head), '@');
    if (it == end(head))
        throw std::runtime_error("can't determine the offset of first article");
    return std::distance(begin(head), it);
}

std::unique_ptr<IResourceArchiveReader> makeArchiveReader(fs::path inputPath,
                                                          const duden::ResourceArchive& archive) {
    if (archive.fsd.empty()) {
        dictlsd::FileStream fIdx((inputPath / archive.idx).string());
        auto fBof = std::make_shared<dictlsd::FileStream>((inputPath / archive.bof).string());
        return std::make_unique<Archive>(&fIdx, fBof);
    } else {
        auto fFsd = std::make_shared<dictlsd::FileStream>((inputPath / archive.fsd).string());
        return std::make_unique<FsdFile>(fFsd);
    }
}
}

void writeDSL(fs::path infPath,
              fs::path outputPath,
              int index,
              Log& log) {
    auto inputPath = infPath.parent_path();
    duden::FileSystem fs(infPath.parent_path().string());
    duden::Dictionary dict(&fs, infPath, index);

    std::string dslFileName = dict.ld().name;

    log.regular("Name:     %s", dict.ld().name);
    log.regular("Articles: %d", dict.articleCount());

    const auto& entries = dict.entries();
    auto groups = groupHicEntries(entries);

    auto overlayPath = outputPath / (dslFileName + ".dsl.files.zip");
    ZipWriter zip(overlayPath.string());

    ResourceFiles resources;
    for (auto& pack : dict.inf().resources) {
        if (!pack.fsi.empty())
            continue;

        log.regular("unpacking %s", pack.bof);

        dictlsd::FileStream fIndex((inputPath / pack.idx).string());
        auto fBof = std::make_shared<dictlsd::FileStream>((inputPath / pack.bof).string());
        Archive archive(&fIndex, fBof);
        std::vector<char> vec;
        archive.read(0, -1, vec);
        auto stream = std::make_unique<std::stringstream>();
        stream->write(vec.data(), vec.size());
        stream->seekg(0);
        auto filename = fs::path(pack.bof).stem().string();
        resources[filename] = std::move(stream);
    }

    dsl::Writer writer(outputPath.string(), dslFileName);
    writer.setName(dictlsd::toUtf16(dict.ld().name));
    writer.setLanguage(dict.ld().sourceLanguageCode, dict.ld().targetLanguageCode);

    std::map<std::string, std::tuple<const ResourceArchive*, uint32_t, uint32_t>> resourceIndex;

    int resourceCount = 0;

    std::vector<std::string> resourceFileNames;

    int adpCount = 0;
    for (auto& pack : dict.inf().resources) {
        if (pack.fsi.empty())
            continue;

        dictlsd::FileStream fFsi((inputPath / pack.fsi).string());
        auto entries = parseFsiFile(&fFsi);

        log.resetProgress(pack.fsi, entries.size());

        auto reader = makeArchiveReader(inputPath, pack);

        std::vector<char> vec;
        int i = 0;
        for (auto& entry : entries) {
            resourceIndex[entry.name] = {&pack, entry.offset, entry.size};
            log.verbose("unpacking [%03d/%03d] %s", i, entries.size(), entry.name);
            if (entry.offset >= reader->decodedSize()) {
                log.regular("resource %s has invalid offset %x", entry.name, entry.offset);
                continue;
            }
            reader->read(entry.offset, entry.size, vec);

            auto name = entry.name;
            if (replaceAdpExtWithWav(name)) {
                std::vector<int16_t> samples(2 * vec.size());
                decodeAdp(vec, &samples[0]);
                dictlsd::createWav(samples, vec, ADP_SAMPLE_RATE);
                adpCount++;
            }

            zip.addFile(name, vec.data(), vec.size());
            resourceFileNames.push_back(name);
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
        auto fBof = std::make_shared<dictlsd::FileStream>((inputPath / pack->bof).string());
        Archive archive(&fIndex, fBof);
        archive.read(offset, size, vec);
        return vec;
    });

    int articleCount = 0;
    int failedArticleCount = 0;

    ResourceFileSystem resourceFS(std::move(resourceFileNames));

    auto bofOffset = calcBofOffset(dict);

    for (const auto& [textOffset, group] : groups) {
        log.advance();
        // headings of pictures, tables, etc
        if (static_cast<unsigned>(textOffset) >= dict.articleArchiveDecodedSize())
            continue;
        ParsingContext context;
        TextRun* headingRun = nullptr;
        for (const auto& heading : group.headings) {
            headingRun = parseDudenText(context, heading);
            auto dslHeading = printDsl(headingRun);
            writer.writeHeading(dictlsd::toUtf16(dslHeading));
        }

        try {
            auto article = dict.article(textOffset + bofOffset, group.articleSize);
            auto articleRun = parseDudenText(context, article);
            resolveReferences(context, articleRun, dict.ld(), &resourceFS);
            inlineReferences(context, articleRun, resources);
            resolveReferences(context, articleRun, dict.ld(), &resourceFS);

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
        } catch (std::exception& e) {
            log.regular("failed to parse article [%s] with error: %s", group.headings.front(), e.what());
            writer.writeArticle(dictlsd::toUtf16("<Parsing error>"));
            failedArticleCount++;
        }
    }

    log.regular("done converting: %d articles (%d errors), %d tables, %d resources, %d audio files",
                articleCount,
                failedArticleCount,
                tableCount,
                resourceCount,
                adpCount);
}

} // namespace duden
