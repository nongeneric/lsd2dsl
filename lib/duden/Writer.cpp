#include "Writer.h"
#include "common/DslWriter.h"
#include "common/ZipWriter.h"
#include "TableRenderer.h"
#include "common/bformat.h"
#include "duden/Duden.h"
#include "duden/Archive.h"
#include "duden/AdpDecoder.h"
#include "duden/FsdFile.h"
#include "duden/HtmlRenderer.h"
#include "duden/text/TextRun.h"
#include "duden/text/Parser.h"
#include "duden/text/Printers.h"
#include "duden/text/Reference.h"
#include "lingvo/tools.h"
#include "common/WavWriter.h"

namespace duden {

using namespace std::literals;

namespace {

class ResourceFileSystem : public IFileSystem {
    CaseInsensitiveSet _paths;

public:
    ResourceFileSystem(const std::vector<std::string>& names) {
        for (auto& name : names) {
            _paths.insert(std::filesystem::u8path(name));
        }
    }

    std::unique_ptr<common::IRandomAccessStream> open(std::filesystem::path) override {
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

std::unique_ptr<IResourceArchiveReader> makeArchiveReader(std::filesystem::path inputPath,
                                                          const duden::ResourceArchive& archive) {
    if (archive.fsd.empty()) {
        common::FileStream fIdx(inputPath / archive.idx);
        auto fBof = std::make_shared<common::FileStream>(inputPath / archive.bof);
        return std::make_unique<Archive>(&fIdx, fBof);
    } else {
        auto fFsd = std::make_shared<common::FileStream>(inputPath / archive.fsd);
        return std::make_unique<FsdFile>(fFsd);
    }
}
}

std::string defaultArticleResolve(const std::map<int32_t, HeadingGroup>& groups,
                                  int64_t offset,
                                  std::string hint,
                                  ParsingContext& context)
{
    auto it = groups.find(offset - 1);
    if (it == end(groups)) {
        return {};
    }
    auto& headings = it->second.headings;
    hint = trimReferenceDisplayName(hint);
    auto exact = std::find(begin(headings), end(headings), hint);
    auto& heading = exact == end(headings) ? headings.front() : *exact;
    return printDsl(parseDudenText(context, heading));
}

void writeDSL(std::filesystem::path infPath,
              std::filesystem::path outputPath,
              int index,
              Log& log) {
    auto inputPath = infPath.parent_path();
    duden::FileSystem fs(infPath.parent_path());
    duden::Dictionary dict(&fs, infPath, index);

    std::string dslFileName = dict.ld().name;

    log.regular("Version:  %x (HIC %d)", dict.inf().version, dict.hic().version);
    log.regular("Name:     %s", dict.ld().name);
    log.regular("Articles: %d", dict.articleCount());

    const auto& entries = dict.entries();
    auto groups = groupHicEntries(entries);

    dsl::Writer writer(outputPath, dslFileName);
    auto overlayPath = std::filesystem::u8path(writer.dslFilePath().u8string() + ".files.zip");
    ZipWriter zip(overlayPath);

    ResourceFiles resources;
    for (auto& pack : dict.inf().resources) {
        if (!pack.fsi.empty())
            continue;

        log.regular("unpacking %s", pack.bof);

        common::FileStream fIndex(inputPath / pack.idx);
        auto fBof = std::make_shared<common::FileStream>(inputPath / pack.bof);
        Archive archive(&fIndex, fBof);
        std::vector<char> vec;
        archive.read(0, -1, vec);
        auto stream = std::make_unique<std::stringstream>();
        stream->write(vec.data(), vec.size());
        stream->seekg(0);
        auto filename = std::filesystem::u8path(pack.bof).stem().u8string();
        resources[filename] = std::move(stream);
    }

    writer.setName(toUtf16(dict.ld().name));
    writer.setLanguage(dict.ld().sourceLanguageCode, dict.ld().targetLanguageCode);

    std::map<std::string, std::tuple<const ResourceArchive*, uint32_t, uint32_t>> resourceIndex;

    int resourceCount = 0;

    std::vector<std::string> resourceFileNames;

    int adpCount = 0;
    for (auto& pack : dict.inf().resources) {
        if (pack.fsi.empty())
            continue;

        common::FileStream fFsi(inputPath / pack.fsi);
        auto entries = parseFsiFile(&fFsi);

        log.resetProgress(pack.fsi, entries.size());

        auto reader = makeArchiveReader(inputPath, pack);

        std::vector<char> vec;
        int i = 0;
        std::vector<int16_t> samples;
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
                decodeAdp(vec, samples);
                common::createWav(samples, vec, ADP_SAMPLE_RATE, ADP_CHANNELS);
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

    TableRenderer tableRenderer([&](auto name) {
            log.verbose("table has embedded image %s", name);
            auto it = resourceIndex.find(name);
            std::vector<char> vec;
            if (it == end(resourceIndex)) {
                log.regular("embedded image %s doesn't exist", name);
                return vec;
            }
            auto [pack, offset, size] = it->second;
            common::FileStream fIndex(inputPath / pack->idx);
            auto fBof = std::make_shared<common::FileStream>(inputPath / pack->bof);
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
            auto dslHeading = printDslHeading(headingRun);
            writer.writeHeading(toUtf16(dslHeading));
        }

        try {
            auto article = dict.article(textOffset + bofOffset, group.articleSize);
            auto articleRun = parseDudenText(context, article);
            resolveReferences(context, articleRun, dict.ld(), &resourceFS);
            inlineReferences(context, articleRun, resources);
            resolveReferences(context, articleRun, dict.ld(), &resourceFS);

            const auto& firstHeading = group.headings.front();

            resolveArticleReferences(articleRun, [&](auto offset, auto& hint) {
                auto heading = defaultArticleResolve(groups, offset, hint, context);
                if (heading.empty()) {
                    log.regular("Article [%s] references unknown article %d", firstHeading, offset - 1);
                    heading = "unknown";
                }
                return heading;
            });
            tableRenderer.render(articleRun);
            if (group.headings.size() == 1) {
                dedupHeading(headingRun, articleRun);
            }
            auto dslArticle = printDsl(articleRun);
            writer.writeArticle(toUtf16(dslArticle));
            ++articleCount;
        } catch (std::exception& e) {
            log.regular("failed to parse article [%s] with error: %s", group.headings.front(), e.what());
            writer.writeArticle(toUtf16("<Parsing error>"));
            failedArticleCount++;
        }
    }

    auto const& htmlTables = tableRenderer.getHtmls();

    if (!htmlTables.empty()) {
        log.resetProgress("rendering tables", htmlTables.size());
        std::vector<std::string const*> htmlTablePtrs;
        for (auto&& [html, _] : htmlTables) {
            htmlTablePtrs.push_back(&html);
        }
        auto tableEntry = htmlTables.begin();
        renderHtml([&](QImage const& image) {
            assert(tableEntry != end(htmlTables));
            log.advance();
            auto bytes = qImageToPng(image);
            zip.addFile(tableEntry->second, bytes.data(), bytes.size());
            tableEntry++;
        }, htmlTablePtrs, log);
    }

    log.regular("done converting: %d articles (%d errors), %d tables, %d resources, %d audio files",
                articleCount,
                failedArticleCount,
                htmlTables.size(),
                resourceCount,
                adpCount);
}

} // namespace duden
