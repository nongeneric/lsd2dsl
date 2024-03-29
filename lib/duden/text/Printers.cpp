#include "Printers.h"

#include "Table.h"
#include "duden/text/Reference.h"
#include <fmt/format.h>
#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm.hpp>
#include <QByteArray>

namespace duden {

namespace {

class TreeVisitor : public TextRunVisitor {
    std::string _result;

    std::string spaces(TextRun* run) {
        std::string res;
        for (;;) {
            run = run->parent();
            if (!run)
                break;
            res += "  ";
        }
        return res;
    }

    template <class T>
    void print(TextRun* run, T label) {
        _result += fmt::format("{}{}\n", spaces(run), label);
    }

public:
    void visit(TextRun* run) override {
        print(run, "TextRun");
        TextRunVisitor::visit(run);
    }

    void visit(BoldFormattingRun* run) override {
        print(run, "BoldFormattingRun");
        TextRunVisitor::visit(run);
    }

    void visit(PlainRun* run) override {
        print(run, fmt::format("PlainRun: {}", run->text()));
        TextRunVisitor::visit(run);
    }
    void visit(ReferenceRun* run) override {
        print(run, "ReferenceRun");
        TextRunVisitor::visit(run);
    }

    void visit(ItalicFormattingRun* run) override {
        print(run, "ItalicFormattingRun");
        TextRunVisitor::visit(run);
    }

    void visit(UnderlineFormattingRun* run) override {
        print(run, "UnderlineFormattingRun");
        TextRunVisitor::visit(run);
    }

    void visit(WebLinkFormattingRun* run) override {
        print(run, "WebLinkFormattingRun");
        TextRunVisitor::visit(run);
    }

    void visit(ColorFormattingRun* run) override {
        print(run,
              fmt::format("ColorFormattingRun; rgb={:06x}; name={}; tilde={:d}",
                          run->rgb(),
                          run->name(),
                          run->tilde()));
        TextRunVisitor::visit(run);
    }

    void visit(AddendumFormattingRun* run) override {
        print(run, "AddendumFormattingRun");
        TextRunVisitor::visit(run);
    }

    void visit(AlignmentFormattingRun* run) override {
        print(run, "AlignmentFormattingRun");
        TextRunVisitor::visit(run);
    }

    void visit(SuperscriptFormattingRun* run) override {
        print(run, "SuperscriptFormattingRun");
        TextRunVisitor::visit(run);
    }

    void visit(SubscriptFormattingRun* run) override {
        print(run, "SubscriptFormattingRun");
        TextRunVisitor::visit(run);
    }

    void visit(TagReferenceRun* run) override {
        print(run, "TagReferenceRun");
        TextRunVisitor::visit(run);
    }

    void visit(ArticleReferenceRun* run) override {
        print(run, fmt::format("ArticleReferenceRun; offset={}", run->offset()));
        TextRunVisitor::visit(run);
    }

    void visit(IdRun* run) override {
        print(run, fmt::format("IdRun: {}", run->id()));
        TextRunVisitor::visit(run);
    }

    void visit(TabRun* run) override {
        print(run, fmt::format("TabRun: {}", run->column()));
        TextRunVisitor::visit(run);
    }

    void visit(ReferencePlaceholderRun* run) override {
        auto range = run->range() ? fmt::format("; range={}-{}", run->range()->from, run->range()->to)
                   : "";
        print(run,
              fmt::format("ReferencePlaceholderRun; code={}; num={}; num2={}{}",
                          run->id().code,
                          run->id().num,
                          run->id().num2,
                          range));
        TextRunVisitor::visit(run);
    }

    void visit(LineBreakRun* run) override {
        print(run, "LineBreakRun");
        TextRunVisitor::visit(run);
    }

    void visit(SoftLineBreakRun* run) override {
        print(run, "SoftLineBreakRun");
        TextRunVisitor::visit(run);
    }

    void visit(TableRun* run) override {
        print(run,
              fmt::format("TableRun{}{}",
                      run->table() ? "" : " (null Table)",
                      run->renderedName().empty() ? "" : " " + run->renderedName()));
        TextRunVisitor::visit(run);
    }

    void visit(TableTag* run) override {
        print(run,
              fmt::format("TableTag {}; from={}; to={}",
                      (int)run->type(),
                      run->from(),
                      run->to()));
        TextRunVisitor::visit(run);
    }

    void visit(TableCellRun* run) override {
        print(run, "TableCellRun");
        TextRunVisitor::visit(run);
    }

    void visit(TableReferenceRun* run) override {
        print(run,
              fmt::format("TableReferenceRun; offset={}; file={}",
                      run->offset(),
                      run->fileName()));
        TextRunVisitor::visit(run);
    }

    void visit(PictureReferenceRun* run) override {
        print(run,
              fmt::format("PictureReferenceRun; offset={}; file={}; cr={}; image={}",
                      run->offset(),
                      run->fileName(),
                      run->copyright(),
                      run->imageFileName()));
        TextRunVisitor::visit(run);
    }

    void visit(WebReferenceRun* run) override {
        print(run, fmt::format("WebReferenceRun; link={}", run->link()));
        TextRunVisitor::visit(run);
    }

    void visit(InlineImageRun* run) override {
        print(run,
              fmt::format("InlineImageRun; name={}; secondary={}",
                      run->name(),
                      run->secondary()));
    }

    void visit(InlineSoundRun* run) override {
        std::vector<std::string> pairs;
        for (auto& name : run->names()) {
            pairs.push_back(fmt::format("({})", name.file));
        }
        auto names = fmt::format("[{}]", boost::algorithm::join(pairs, ", "));
        print(run, fmt::format("InlineSoundRun; name={}", names));
        TextRunVisitor::visit(run);
    }

    void visit(StickyRun* run) override {
        print(run, fmt::format("StickyRun; num={}", run->num()));
    }

    const std::string& result() const { return _result; }
};

class HtmlVisitor : public TextRunVisitor {
    std::string _result;
    duden::RequestImageCallback _requestImage;

    template <class T>
    void visitTag(T run, const char* tag) {
        _result += "<";
        _result += tag;
        _result += ">";
        TextRunVisitor::visit(run);
        _result += "</";
        _result += tag;
        _result += ">";
    }

    void visit(BoldFormattingRun* run) override {
        visitTag(run, "b");
    }

    void visit(PlainRun* run) override {
        _result += run->text();
    }

    void visit(ItalicFormattingRun* run) override {
        visitTag(run, "i");
    }

    void visit(UnderlineFormattingRun* run) override {
        visitTag(run, "u");
    }

    void visit(SuperscriptFormattingRun* run) override {
        visitTag(run, "sup");
    }

    void visit(SubscriptFormattingRun* run) override {
        visitTag(run, "sub");
    }

    void visit(AddendumFormattingRun* run) override {
        _result += "(";
        TextRunVisitor::visit(run);
        _result += ")";
    }

    void visit(ColorFormattingRun* run) override {
        if (run->tilde()) {
            TextRunVisitor::visit(run);
            return;
        }
        _result += "<font color=\"";
        _result += run->name();
        _result += "\">";
        TextRunVisitor::visit(run);
        _result += "</font>";
    }

    void visit(TableRun* run) override {
        if (!run->table())
            throw std::runtime_error("TableRun doesn't have an associated Table");

        auto table = run->table();
        auto columnWidth = 100 / table->columns();
        _result += "<table>";
        for (auto r = 0; r < table->rows(); ++r) {
            _result += "<tr>";
            for (auto c = 0; c < table->columns(); ++c) {
                if (table->merged(r, c))
                    continue;
                auto cell = table->cell(r, c);
                auto hspan = table->hspan(r, c);
                _result += fmt::format("<td style=\"width: {}%;\" colspan=\"{}\" rowspan=\"{}\">",
                                   columnWidth * hspan,
                                   hspan,
                                   table->vspan(r, c));
                cell->accept(this);
                _result += "</td>";
            }
            _result += "</tr>";
        }
        _result += "</table>";
    }

    void visit(InlineImageRun* run) override {
        auto image = _requestImage(run->name());
        auto ext = std::filesystem::u8path(run->name()).extension().u8string();
        if (ext.size() < 1)
            throw std::runtime_error("inlining resource without extension");
        boost::algorithm::to_lower(ext);
        ext = ext.substr(1);
        auto array = QByteArray::fromRawData(reinterpret_cast<char*>(image.data()), image.size());
        auto base64 = array.toBase64();
        std::string str(base64.data(), base64.size());
        _result += fmt::format("<img src=\"data:image/{};base64,{}\">", ext, str);
    }

    void visit(LineBreakRun*) override {
        _result += "<br>";
    }

public:
    HtmlVisitor(duden::RequestImageCallback requestImage) : _requestImage(requestImage) {}
    const std::string& result() const { return _result; }
};

class DslVisitor : public TextRunVisitor {
    std::string _result;
    std::string _separator = "----------";

    template <class T>
    void visitTag(T run, const char* tag) {
        _result += "[";
        _result += tag;
        _result += "]";
        TextRunVisitor::visit(run);
        _result += "[/";
        _result += tag;
        _result += "]";
    }

    void visit(BoldFormattingRun* run) override {
        visitTag(run, "b");
    }

    void visit(PlainRun* run) override {
        auto text = run->text();
        boost::algorithm::replace_all(text, "[", "\\[");
        boost::algorithm::replace_all(text, "]", "\\]");
        boost::algorithm::replace_all(text, "#", "\\#");
        boost::algorithm::replace_all(text, "~", "\\~");
        boost::algorithm::replace_all(text, "^", "\\^");
        boost::algorithm::replace_all(text, "@", "\\@");
        _result += text;
    }

    void visit(ItalicFormattingRun* run) override {
        visitTag(run, "i");
    }

    void visit(UnderlineFormattingRun* run) override {
        visitTag(run, "u");
    }

    void visit(WebLinkFormattingRun* run) override {
        visitTag(run, "url");
    }

    void visit(SuperscriptFormattingRun* run) override {
        visitTag(run, "sup");
    }

    void visit(SubscriptFormattingRun* run) override {
        visitTag(run, "sub");
    }

    void visit(ColorFormattingRun* run) override {
        if (run->tilde()) {
            TextRunVisitor::visit(run);
            return;
        }
        _result += "[c ";
        _result += run->name();
        _result += "]";
        TextRunVisitor::visit(run);
        _result += "[/c]";
    }

    void visit(AddendumFormattingRun* run) override {
        _result += "(";
        TextRunVisitor::visit(run);
        _result += ")";
    }

    void visit(AlignmentFormattingRun* run) override {
        TextRunVisitor::visit(run);
    }

    void visit(LineBreakRun*) override {
        _result += "[br]\n";
    }

    void visit(SoftLineBreakRun*) override {
        _result += " ";
    }

    void visit(ReferencePlaceholderRun*) override { }

    void visit(TableRun* run) override {
        assert(!run->renderedName().empty() &&
               "trying to print TableRun without rendering first");
        _result += fmt::format("\n[s]{}[/s]", run->renderedName());
    }

    void visit(TableReferenceRun* run) override {
        _result += "\n";
        _result += _separator;
        _result += "\n";
        TextRunVisitor::visit(run->referenceCaption());
        _result += "\n";
        TextRunVisitor::visit(run->content());
        _result += _separator;
        _result += "\n";
    }

    void visit(PictureReferenceRun* run) override {
        _result += "\n";
        _result += _separator;
        _result += "\n";
        TextRunVisitor::visit(run->header());
        _result += "\n";
        TextRunVisitor::visit(run->description());
        _result += "\n";
        _result += fmt::format("[s]{}[/s]", run->imageFileName());
        _result += "\n";
        _result += run->copyright();
        _result += "\n";
        _result += _separator;
        _result += "\n";
    }

    void visit(ArticleReferenceRun* run) override {
        const auto& headingName = run->heading();
        auto headingCaption = printDsl(run->caption());
        auto trimmedCaption = trimReferenceDisplayName(headingCaption);
        auto captionBody = headingCaption.find(trimmedCaption);
        assert(captionBody != std::string::npos);

        auto head = headingCaption.substr(0, captionBody);
        auto tail = headingCaption.substr(captionBody + trimmedCaption.size());

        if (headingName == trimmedCaption) {
            _result += fmt::format("{}[ref]{}[/ref]{}", head, headingName, tail);
        } else {
            _result += fmt::format("{}[ref]{}[/ref] ({}){}", head, headingName, trimmedCaption, tail);
        }
    }

    void visit(InlineImageRun* run) override {
        const auto& file = run->secondary().empty() ? run->name() : run->secondary();
        _result += fmt::format("[s]{}[/s]", file);
    }

    void visit(InlineSoundRun* run) override {
        auto& names = run->names();
        for (size_t i = 0; i < names.size(); ++i) {
            if (i != 0) {
                _result += ", ";
            }
            _result += fmt::format("[s]{}[/s]", names[i].file);
            if (names[i].label) {
                TextRunVisitor::visit(names[i].label);
            }
        }
        _result += " ";
    }

public:
    const std::string& result() const { return _result; }
};

class ConsecutiveNewLineRewriter : public TextRunVisitor {
public:
    void visit(TextRun* run) override {
        auto& runs = run->runs();
        auto isNewLine = [] (auto textRun) {
            return dynamic_cast<SoftLineBreakRun*>(textRun)
                || dynamic_cast<LineBreakRun*>(textRun);
        };

        auto isInvisible = [] (auto textRun) -> bool {
            return dynamic_cast<ReferencePlaceholderRun*>(textRun)
                || dynamic_cast<IdRun*>(textRun);
        };

        boost::range::remove_erase_if(runs, isInvisible);
        runs.erase(begin(runs), boost::find_if(runs, [=] (auto run) { return !isNewLine(run); }));

        auto it = begin(runs);
        while (it != end(runs)) {
            TextRun* lineBreak = nullptr;
            TextRun* softLineBreak = nullptr;
            auto start = std::find_if(it, end(runs), isNewLine);
            it = start;
            while (it != end(runs) && isNewLine(*it)) {
                if (auto typed = dynamic_cast<LineBreakRun*>(*it))
                    lineBreak = typed;
                if (auto typed = dynamic_cast<SoftLineBreakRun*>(*it))
                    softLineBreak = typed;
                ++it;
            }
            if (it - start < 2)
                continue;
            if (lineBreak) {
                std::replace_if(start, it, [] (auto run) { return dynamic_cast<SoftLineBreakRun*>(run); }, nullptr);
            } else {
                std::fill(start, it, nullptr);
                *start = softLineBreak;
            }
        }

        boost::range::remove_erase(runs, nullptr);

        for (auto child : runs) {
            child->accept(this);
        }
    }
};

}

std::string printTree(TextRun *run) {
    TreeVisitor visitor;
    run->accept(&visitor);
    return visitor.result();
}

std::string printHtml(TextRun *run, RequestImageCallback requestImage) {
    HtmlVisitor htmlVisitor(requestImage);
    run->accept(&htmlVisitor);
    return htmlVisitor.result();
}

std::string printDsl(TextRun *run) {
    ConsecutiveNewLineRewriter newLineRewriter;
    run->accept(&newLineRewriter);
    DslVisitor dslVisitor;
    run->accept(&dslVisitor);
    return dslVisitor.result();
}

std::string printDslHeading(TextRun* run) {
    auto heading = printDsl(run);
    boost::replace_all(heading, "(", "\\(");
    boost::replace_all(heading, ")", "\\)");
    return heading;
}

}
