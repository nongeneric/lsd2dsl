#include "Reference.h"

#include "lib/duden/Duden.h"
#include "Parser.h"
#include "lib/common/bformat.h"
#include "lib/duden/AdpDecoder.h"
#include <boost/algorithm/string.hpp>

#include "lib/duden/text/Printers.h"

namespace duden {

namespace {

class ReferenceResolverRewriter : public TextRunVisitor {
    ParsingContext& _context;
    const LdFile& _ld;
    IFileSystem* _filesystem;

    std::tuple<std::string, uint32_t> findFileName(int64_t offset) {
        for (auto& range : _ld.ranges) {
            if (range.first <= offset && offset < range.last) {
                return {range.fileName, static_cast<uint32_t>(offset - range.first)};
            }
        }
        throw std::runtime_error("unknown reference range");
    }

    void fixCase(std::string& file) {
        if (!_filesystem)
            return;
        auto& files = _filesystem->files();
        auto it = files.find(fs::path(file));
        if (it != end(files)) {
            file = it->string();
        }
    }

    void visit(InlineSoundRun* run) override {
        if (!run->names().empty())
            return; // already resolved

        std::vector<InlineSoundName> names;
        for (auto run : run->runs()) {
            auto plain = dynamic_cast<PlainRun*>(run->runs().front());
            if (!plain)
                throw std::runtime_error("misformed InlineSoundRun");

            auto file = plain->text();
            auto idx = file.find(" \"");
            if (idx != std::string::npos) {
                file = file.substr(0, idx);
                run->replace(plain, _context.make<PlainRun>(plain->text().substr(idx)));
            } else {
                run->replace(plain, _context.make<PlainRun>(""));
            }

            replaceAdpExtWithWav(file);
            names.push_back({file, run});
        }

        run->setNames(std::move(names));
    }

    void visit(ReferencePlaceholderRun* run) override {
        auto code = run->id().code;
        TextRun* newRun = run;

        if (code.empty()) {
            auto child = run->runs().front();
            if (!child->runs().empty() && run->id().num != -1) {
                newRun = _context.make<ArticleReferenceRun>(child, run->id().num);
            }
        } else {
            auto prefix = code[0];
            code = code.substr(1);

            if (prefix == 'I') {
                std::string secondary;
                auto runs = run->runs();
                if (runs.size() > 2) {
                    auto first = dynamic_cast<PlainRun*>(runs[1]);
                    auto second = dynamic_cast<PlainRun*>(runs[2]);
                    if (first && first->text() == "T" && second) {
                        secondary = second->text();
                        replaceAdpExtWithWav(secondary);
                    }
                }
                fixCase(code);
                fixCase(secondary);
                newRun = _context.make<InlineImageRun>(code, secondary);
            } else {
                auto ref = std::find_if(begin(_ld.references), end(_ld.references), [&](auto& info) {
                    return info.code == code;
                });
                if (ref != end(_ld.references)) {
                    if (prefix != 'M')
                        return;
                    if (ref->name == "Tabellen") {
                        auto [fileName, offset] = findFileName(run->id().num);
                        auto caption = run->runs().front();
                        newRun = _context.make<TableReferenceRun>(offset, fileName, caption);
                    } else if (ref->name == "Bilder") {
                        auto [fileName, offset] = findFileName(run->id().num);
                        auto caption = run->runs().back();
                        newRun = _context.make<PictureReferenceRun>(offset, fileName, caption);
                    } else if (ref->name == "Web") {
                        auto caption = run->runs().front();
                        auto link = dynamic_cast<PlainRun*>(run->runs().back());
                        if (!link) {
                            // weblink is misformed
                            return;
                        }
                        newRun = _context.make<WebReferenceRun>(link->text(), caption);
                    }
                }
            }
        }
        run->parent()->replace(run, newRun);
    }

public:
    ReferenceResolverRewriter(ParsingContext& context, LdFile const& ld, IFileSystem* filesystem)
        : _context(context), _ld(ld), _filesystem(filesystem) {}
};

class ReferenceInlinerRewriter : public TextRunVisitor {
    ParsingContext& _context;
    const ResourceFiles& _files;

    struct PictureInfo {
        std::string copyright;
        std::string file;
        std::string type;
        std::string unnamed;
        TextRun* header;
        TextRun* description;
    };

    struct TableInfo {
        std::string mt;
        TextRun* content;
    };

    PictureInfo parsePicture(std::istream& stream) {
        PictureInfo info;
        std::string line;
        std::getline(stream, line);
        info.header = parseDudenText(_context, line);

        std::string crPrefix = "@C%CR=";
        std::string filePrefix = "@C%File=";
        std::string typePrefix = "@C%Type=";
        std::string unnamedPrefix = "@C";

        auto parsePrefix = [&] (auto& prefix, auto& member) {
            if (boost::algorithm::starts_with(line, prefix)) {
                member = line.substr(prefix.size());
                return true;
            }
            return false;
        };

        for (;;) {
            std::getline(stream, line);
            line = dudenToUtf8(line);

            if (parsePrefix(crPrefix, info.copyright) ||
                parsePrefix(filePrefix, info.file) ||
                parsePrefix(typePrefix, info.type) ||
                parsePrefix(unnamedPrefix, info.unnamed))
                continue;

            if (boost::algorithm::starts_with(line, "@C%"))
                throw std::runtime_error("unknown picture attribute"); // warning

            info.description = parseDudenText(_context, line);
            break;
        }

        return info;
    }

    TableInfo parseTable(std::istream& stream) {
        TableInfo info;
        std::string line;
        std::string rawTable;
        std::string mtPrefix = "@C%MT=";

        for (;;) {
            if (!std::getline(stream, line))
                throw std::runtime_error("unexpected end of file");

            line = dudenToUtf8(line);

            if (boost::algorithm::starts_with(line, mtPrefix)) {
                info.mt = line.substr(mtPrefix.size());
                boost::algorithm::trim_if(info.mt, boost::algorithm::is_any_of("\""));
                continue;
            }

            rawTable += line;
            rawTable += "\n";
            if (line.find("\\S{;:") == 0)
                break;
        }

        info.content = parseDudenText(_context, rawTable);
        return info;
    }

    std::istream& getStream(std::string fileName) {
        auto file = _files.find(fileName);
        if (file == end(_files))
            throw std::runtime_error("reference into unknown file");
        return *file->second;
    }

    void visit(TableReferenceRun* run) override {
        auto& stream = getStream(run->fileName());
        stream.seekg(run->offset());
        auto info = parseTable(stream);
        run->setContent(info.content);
        run->setMt(info.mt);
    }

    void visit(PictureReferenceRun* run) override {
        auto& stream = getStream(run->fileName());
        stream.seekg(run->offset());
        auto info = parsePicture(stream);
        run->setCopyright(info.copyright);
        run->setDescription(info.description);
        run->setImageFileName(info.file);
        run->setHeader(info.header);
    }

public:
    ReferenceInlinerRewriter(ParsingContext& context, const ResourceFiles& files)
        : _context(context), _files(files) {}
};

class ArticleReferenceVisitor : public TextRunVisitor {
    ResolveArticle _resolveArticle;

public:
    ArticleReferenceVisitor(ResolveArticle resolveArticle) : _resolveArticle(resolveArticle) {}

    void visit(ArticleReferenceRun* run) override {
        run->setHeading(_resolveArticle(run->offset()));
    }
};

} // namespace

void resolveReferences(ParsingContext& context, TextRun* run, LdFile const& ld, IFileSystem* filesystem) {
    ReferenceResolverRewriter rewriter(context, ld, filesystem);
    run->accept(&rewriter);
}

void inlineReferences(ParsingContext &context, TextRun *run, const ResourceFiles& files) {
    ReferenceInlinerRewriter rewriter(context, files);
    run->accept(&rewriter);
}

void resolveArticleReferences(TextRun *run, ResolveArticle resolveArticle) {
    ArticleReferenceVisitor visitor(resolveArticle);
    run->accept(&visitor);
}

} // namespace duden
