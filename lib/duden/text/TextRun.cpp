#include "TextRun.h"

#include "Table.h"
#include "Parser.h"
#include "lib/common/bformat.h"
#include "lib/duden/Duden.h"
#include <boost/algorithm/string.hpp>
#include <assert.h>
#include <algorithm>

namespace duden {

void FormattingRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void BoldFormattingRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void BoldItalicFormattingRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void ItalicFormattingRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void UnderlineFormattingRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void WebLinkFormattingRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void ColorFormattingRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void StickyRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void AddendumFormattingRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void AlignmentFormattingRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void SuperscriptFormattingRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void SubscriptFormattingRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void ReferenceRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void TagReferenceRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void ArticleReferenceRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void IdRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void TabRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void LineBreakRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void SoftLineBreakRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void PlainRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void TextRun::replace(TextRun *child, TextRun *run) {
    auto it = std::find(begin(_runs), end(_runs), child);
    assert(it != end(_runs));
    *it = run;
    run->setParent(this);
}

void TextRun::accept(TextRunVisitor* visitor) {
    visitor->visit(this);
}

void ReferencePlaceholderRun::accept(TextRunVisitor *visitor) {
    visitor->visit(this);
}

TableRun::~TableRun() { }

void TableRun::setTable(std::unique_ptr<Table> table) {
    _table = std::move(table);
}

void TableRun::accept(TextRunVisitor *visitor) {
    visitor->visit(this);
}

void TableTag::accept(TextRunVisitor *visitor) {
    visitor->visit(this);
}

void TableCellRun::accept(TextRunVisitor *visitor) {
    visitor->visit(this);
}

void WebReferenceRun::accept(TextRunVisitor *visitor) {
    visitor->visit(this);
}

void PictureReferenceRun::accept(TextRunVisitor *visitor) {
    visitor->visit(this);
}

void TableReferenceRun::accept(TextRunVisitor *visitor) {
    visitor->visit(this);
}

void InlineImageRun::accept(TextRunVisitor *visitor) {
    visitor->visit(this);
}

void InlineSoundRun::accept(TextRunVisitor *visitor) {
    visitor->visit(this);
}

class DedupVisitor : public TextRunVisitor {
    std::string _result;

public:
    void visit(PlainRun* run) override {
        _result += run->text();
    }

    std::string result() const {
        return _result;
    }

    void clear() {
        _result.clear();
    }
};

bool dedupHeading(TextRun* heading, TextRun* article) {
    DedupVisitor visitor;
    heading->accept(&visitor);
    visitor.result();
    auto headingStr = visitor.result();
    visitor.clear();
    std::string articleStr;
    auto& runs = article->runs();
    for (auto it = begin(runs); it != end(runs); ++it) {
        (*it)->accept(&visitor);
        auto result = visitor.result();
        if (articleStr.empty()) {
            boost::algorithm::trim_left(result);
        }
        articleStr += result;
        if (articleStr.size() > headingStr.size())
            return false;
        if (articleStr == headingStr) {
            it = std::find_if(it + 1, end(runs), [](auto run) {
                return !dynamic_cast<LineBreakRun*>(run)
                    && !dynamic_cast<SoftLineBreakRun*>(run);
            });
            runs.erase(begin(runs), it);
            return true;
        }
    }
    return false;
}

} // namespace duden
