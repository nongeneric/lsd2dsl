#include "Reference.h"

#include "duden/Duden.h"
#include "Parser.h"
#include "tools/bformat.h"
#include <boost/algorithm/string.hpp>

namespace duden {

namespace {

class ReferenceResolverRewriter : public TextRunVisitor {
    ParsingContext& _context;
    const LdFile& _ld;

    std::tuple<std::string, uint32_t> findFileName(int64_t offset) {
        for (auto& range : _ld.ranges) {
            if (range.first <= offset && offset < range.last) {
                return {range.fileName, static_cast<uint32_t>(offset - range.first)};
            }
        }
        throw std::runtime_error("unknown reference range");
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
                newRun = _context.make<InlineImageRun>(code);
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
    ReferenceResolverRewriter(ParsingContext& context, LdFile const& ld)
        : _context(context), _ld(ld) {}
};

class ReferenceInlinerRewriter : public TextRunVisitor {
    ParsingContext& _context;
    ResourceFiles& _files;

    struct PictureInfo {
        std::string copyright;
        std::string file;
        std::string type;
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
                parsePrefix(typePrefix, info.type))
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
            std::getline(stream, line);
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
    ReferenceInlinerRewriter(ParsingContext& context, ResourceFiles& files)
        : _context(context), _files(files) {}
};

} // namespace

void resolveReferences(ParsingContext& context, TextRun* run, LdFile const& ld) {
    ReferenceResolverRewriter rewriter(context, ld);
    run->accept(&rewriter);
}

void inlineReferences(ParsingContext &context, TextRun *run, ResourceFiles& files) {
    ReferenceInlinerRewriter rewriter(context, files);
    run->accept(&rewriter);
}

} // namespace duden
