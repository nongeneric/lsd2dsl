#include "Printers.h"

#include "Table.h"
#include "lib/common/bformat.h"
#include "lib/common/filesystem.h"
#include <boost/algorithm/string.hpp>
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
        _result += bformat("%s%s\n", spaces(run), label);
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
        print(run, bformat("PlainRun: %s", run->text()));
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
              bformat("ColorFormattingRun; rgb=%06x; name=%s",
                      run->rgb(),
                      run->name()));
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
        print(run,
              bformat("ArticleReferenceRun; offset=%d",
                      run->offset()));
        TextRunVisitor::visit(run);
    }

    void visit(IdRun* run) override {
        print(run, bformat("IdRun: %d", run->id()));
        TextRunVisitor::visit(run);
    }

    void visit(TabRun* run) override {
        print(run, bformat("TabRun: %d", run->column()));
        TextRunVisitor::visit(run);
    }

    void visit(ReferencePlaceholderRun* run) override {
        auto range = run->range() ? bformat("; range=%d-%d", run->range()->from, run->range()->to)
                   : "";
        print(run,
              bformat("ReferencePlaceholderRun; code=%s; num=%d; num2=%d%s",
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
        print(run, bformat("TableRun%s", run->table() ? "" : " (null Table)"));
        TextRunVisitor::visit(run);
    }

    void visit(TableTag* run) override {
        print(run,
              bformat("TableTag %d; from=%d; to=%d",
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
              bformat("TableReferenceRun; offset=%d; file=%s",
                      run->offset(),
                      run->fileName()));
        TextRunVisitor::visit(run);
    }

    void visit(PictureReferenceRun* run) override {
        print(run,
              bformat("PictureReferenceRun; offset=%d; file=%s; cr=%s; image=%s",
                      run->offset(),
                      run->fileName(),
                      run->copyright(),
                      run->imageFileName()));
        TextRunVisitor::visit(run);
    }

    void visit(WebReferenceRun* run) override {
        print(run,
              bformat("WebReferenceRun; link=%s",
                      run->link()));
        TextRunVisitor::visit(run);
    }

    void visit(InlineImageRun* run) override {
        print(run,
              bformat("InlineImageRun; name=%s; secondary=%s",
                      run->name(),
                      run->secondary()));
    }

    void visit(InlineSoundRun* run) override {
        std::vector<std::string> pairs;
        for (auto& name : run->names()) {
            pairs.push_back(bformat("(%s)", name.file));
        }
        auto names = bformat("[%s]", boost::algorithm::join(pairs, ", "));
        print(run, bformat("InlineSoundRun; name=%s", names));
        TextRunVisitor::visit(run);
    }

    void visit(StickyRun* run) override {
        print(run,
              bformat("StickyRun; num=%s",
                      run->num()));
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
                _result += bformat("<td style=\"width: %d%%;\" colspan=\"%d\" rowspan=\"%d\">",
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
        auto ext = fs::path(run->name()).extension().string();
        if (ext.size() < 1)
            throw std::runtime_error("inlining resource without extension");
        boost::algorithm::to_lower(ext);
        ext = ext.substr(1);
        auto array = QByteArray::fromRawData(reinterpret_cast<char*>(&image[0]), image.size());
        auto base64 = array.toBase64();
        std::string str(base64.data(), base64.size());
        _result += bformat("<img src=\"data:image/%s;base64,%s\">", ext, str);
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
        _result += text;
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

    void visit(ColorFormattingRun* run) override {
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
        _result += "\n";
    }

    void visit(SoftLineBreakRun*) override { }

    void visit(ReferencePlaceholderRun*) override { }

    void visit(TableRun* run) override {
        assert(!run->renderedName().empty() &&
               "trying to print TableRun without rendering first");
        _result += bformat("[s]%s[/s]", run->renderedName());
    }

    void visit(TableReferenceRun* run) override {
        _result += _separator;
        _result += "\n";
        TextRunVisitor::visit(run->referenceCaption());
        _result += "\n";
        TextRunVisitor::visit(run->content());
        _result += "\n";
        _result += _separator;
        _result += "\n";
    }

    void visit(PictureReferenceRun* run) override {
        _result += _separator;
        _result += "\n";
        TextRunVisitor::visit(run->header());
        _result += "\n";
        TextRunVisitor::visit(run->description());
        _result += "\n";
        _result += bformat("[s]%s[/s]", run->imageFileName());
        _result += "\n";
        _result += run->copyright();
        _result += "\n";
        _result += _separator;
        _result += "\n";
    }

    void visit(ArticleReferenceRun* run) override {
        const auto& headingName = run->heading();
        auto headingCaption = printDsl(run->caption());
        if (headingName == headingCaption) {
            _result += bformat("[ref]%s[/ref]", headingName);
        } else {
            _result += bformat("[ref]%s[/ref] (%s)", headingName, headingCaption);
        }
    }

    void visit(InlineImageRun* run) override {
        const auto& file = run->secondary().empty() ? run->name() : run->secondary();
        _result += bformat("[s]%s[/s]", file);
    }

    void visit(InlineSoundRun* run) override {
        auto& names = run->names();
        for (size_t i = 0; i < names.size(); ++i) {
            if (i != 0) {
                _result += ", ";
            }
            _result += bformat("[s]%s[/s]", names[i].file);
            if (names[i].label) {
                TextRunVisitor::visit(names[i].label);
            }
        }
        _result += " ";
    }

public:
    const std::string& result() const { return _result; }
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
    DslVisitor htmlVisitor;
    run->accept(&htmlVisitor);
    auto result = htmlVisitor.result();
    result.erase(std::unique(begin(result), end(result), [](auto a, auto b) {
        return a == '\n' && a == b;
    }), end(result));
    return result;
}

}
