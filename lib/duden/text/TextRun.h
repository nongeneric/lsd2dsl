#pragma once

#include "lib/duden/LdFile.h"
#include <vector>
#include <string>
#include <memory>
#include <assert.h>
#include <map>

namespace duden {

class TextRunVisitor;

class TextRun {
    std::vector<TextRun*> _runs;
    TextRun* _parent;

public:
    virtual ~TextRun() = default;

    TextRun* parent() const {
        return _parent;
    }

    void setParent(TextRun* parent) {
        _parent = parent;
    }

    std::vector<TextRun*>& runs() {
        return _runs;
    }

    void addRun(TextRun* run) {
        run->setParent(this);
        _runs.push_back(run);
    }

    void replace(TextRun* child, TextRun* run);

    virtual void accept(TextRunVisitor* visitor);
};

class FormattingRun : public TextRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

class BoldFormattingRun : public FormattingRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

class BoldItalicFormattingRun : public FormattingRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

class ItalicFormattingRun : public FormattingRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

class UnderlineFormattingRun : public FormattingRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

class WebLinkFormattingRun : public FormattingRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

class ColorFormattingRun : public FormattingRun {
    uint32_t _rgb;
    std::string _name;

public:
    ColorFormattingRun(uint32_t rgb, std::string name)
        : _rgb(rgb), _name(name) {}

    void accept(TextRunVisitor *visitor) override;

    uint32_t rgb() const { return _rgb; }
    const std::string& name() const { return _name; }
};

class AddendumFormattingRun : public FormattingRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

class SuperscriptFormattingRun : public FormattingRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

class SubscriptFormattingRun : public FormattingRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

class StickyRun : public TextRun {
    int _num;

public:
    StickyRun(int num) : _num(num) {}
    int num() const { return _num; }
    void accept(TextRunVisitor *visitor) override;
};

class ReferenceRun : public TextRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

class TagReferenceRun : public ReferenceRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

class ArticleReferenceRun : public ReferenceRun {
    TextRun* _caption;
    int64_t _offset;
    std::string _heading;

public:
    void accept(TextRunVisitor *visitor) override;

    ArticleReferenceRun(TextRun* caption, int64_t offset)
        : _caption(caption), _offset(offset) {
        addRun(caption);
    }

    int64_t offset() const { return _offset; }
    TextRun* caption() const { return _caption; }

    void setHeading(std::string heading) {
        _heading = std::move(heading);
    }

    const std::string& heading() {
        return _heading;
    }
};

class Table;

class TableRun : public ReferenceRun {
    std::unique_ptr<Table> _table;
    std::string _renderedName;

public:
    ~TableRun();

    void setTable(std::unique_ptr<Table> table);

    Table* table() const {
        return _table.get();
    }

    void setRenderedName(std::string name) {
        _renderedName = std::move(name);
    }

    const std::string& renderedName() const {
        return _renderedName;
    }

    void accept(TextRunVisitor *visitor) override;
};

enum class TableTagType {
    None, ColumnCount, RowCount,
    tau, tcd, tfl, tcl, tcr,
    tld, tfu, tlt, tcc, ter,
    tcm, ted,
};

class TableTag : public ReferenceRun {
    TableTagType _type = TableTagType::None;
    int _from, _to;

public:
    TableTag(TableTagType type, int from = -1, int to = -1)
        : _type(type), _from(from), _to(to) {}

    int from() const { return _from; }
    int to() const { return _to; }
    TableTagType type() const { return _type; }
    void accept(TextRunVisitor *visitor) override;
};

class TableCellRun : public ReferenceRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

struct ReferenceId {
    std::string code;
    int64_t num;
    int64_t num2;
};

class ReferencePlaceholderRun : public TextRun {
    ReferenceId _id;

public:
    void accept(TextRunVisitor *visitor) override;

    void setId(ReferenceId id) {
        _id = id;
    }

    const ReferenceId& id() const {
        return _id;
    }
};

class TableReferenceRun : public TextRun {
    uint32_t _offset;
    std::string _fileName;
    std::string _mt;
    TextRun* _referenceCaption = nullptr;
    TextRun* _content = nullptr;

public:
    TableReferenceRun(uint32_t offset, std::string fileName, TextRun* referenceCaption)
        : _offset(offset), _fileName(fileName), _referenceCaption(referenceCaption) {
        addRun(referenceCaption);
    }

    void accept(TextRunVisitor *visitor) override;

    uint32_t offset() const {
        return _offset;
    }

    const std::string& fileName() const {
        return _fileName;
    }

    TextRun* referenceCaption() const {
        return _referenceCaption;
    }

    void setContent(TextRun* content) {
        _content = content;
    }

    TextRun* content() {
        return _content;
    }

    void setMt(std::string mt) {
        _mt = mt;
    }

    std::string mt() {
        return _mt;
    }
};

class PictureReferenceRun : public TextRun {
    uint32_t _offset;
    std::string _fileName;
    std::string _copyright;
    std::string _imageFile;
    TextRun* _inlineCaption = nullptr;
    TextRun* _description = nullptr;
    TextRun* _header = nullptr;

public:
    PictureReferenceRun(uint32_t offset, std::string fileName, TextRun* inlineCaption)
        : _offset(offset), _fileName(fileName), _inlineCaption(inlineCaption) {
        addRun(inlineCaption);
    }

    void accept(TextRunVisitor *visitor) override;

    uint32_t offset() const {
        return _offset;
    }

    const std::string& fileName() const {
        return _fileName;
    }

    TextRun* inlineCaption() const {
        return _inlineCaption;
    }

    void setImageFileName(const std::string& name) {
        _imageFile = name;
    }

    void setCopyright(const std::string& copyright) {
        _copyright = copyright;
    }

    void setDescription(TextRun* description) {
        _description = description;
    }

    void setHeader(TextRun* header) {
        _header = header;
    }

    const std::string& imageFileName() {
        return _imageFile;
    }

    const std::string& copyright() {
        return _copyright;
    }

    TextRun* description() {
        return _description;
    }

    TextRun* header() {
        return _header;
    }
};

class WebReferenceRun : public TextRun {
    std::string _link;
    TextRun* _caption;

public:
    WebReferenceRun(std::string link, TextRun* caption) :
        _link(link), _caption(caption) {
        addRun(caption);
    }

    void accept(TextRunVisitor *visitor) override;

    const std::string& link() const {
        return _link;
    }

    TextRun* caption() const {
        return _caption;
    }
};

class InlineImageRun : public TextRun {
    std::string _name;

public:
    InlineImageRun(std::string name) :
        _name(name) { }

    void accept(TextRunVisitor *visitor) override;

    const std::string& name() const {
        return _name;
    }
};

class PictureDescriptionRun : public TextRun {
public:
};

class IdRun : public TextRun {
    int64_t _id{};

public:
    IdRun(int64_t id) : _id(id) {}

    int64_t id() const { return _id; }

    void accept(TextRunVisitor *visitor) override;
};

class TabRun : public TextRun {
    int _column;

public:
    TabRun(int column) : _column(column) {}

    int column() const { return _column; }

    void accept(TextRunVisitor *visitor) override;
};

class LineBreakRun : public TextRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

class SoftLineBreakRun : public TextRun {
public:
    void accept(TextRunVisitor *visitor) override;
};

class PlainRun : public TextRun {
    std::string _text;

public:
    PlainRun(std::string text) : _text(text) { }

    const std::string& text() const {
        return _text;
    }

    void accept(TextRunVisitor *visitor) override;
};

class ParsingContext {
    std::vector<std::unique_ptr<TextRun>> _runs;

public:
    template <class T, class... Args>
    T* make(Args... args) {
        auto run = std::make_unique<T>(args...);
        auto raw = run.get();
        _runs.push_back(std::move(run));
        return raw;
    }
};

class TextRunVisitor {
    void visitImpl(TextRun* run) {
        for (auto child : run->runs()) {
            child->accept(this);
        }
    }

public:
    virtual ~TextRunVisitor() = default;
    virtual void visit(TextRun* run) { visitImpl(run); }
    virtual void visit(FormattingRun* run) { visitImpl(run); }
    virtual void visit(BoldFormattingRun* run) { visitImpl(run); }
    virtual void visit(PlainRun* run) { visitImpl(run); }
    virtual void visit(ReferenceRun* run) { visitImpl(run); }
    virtual void visit(BoldItalicFormattingRun* run) { visitImpl(run); }
    virtual void visit(ItalicFormattingRun* run) { visitImpl(run); }
    virtual void visit(UnderlineFormattingRun* run) { visitImpl(run); }
    virtual void visit(WebLinkFormattingRun* run) { visitImpl(run); }
    virtual void visit(ColorFormattingRun* run) { visitImpl(run); }
    virtual void visit(AddendumFormattingRun* run) { visitImpl(run); }
    virtual void visit(SuperscriptFormattingRun* run) { visitImpl(run); }
    virtual void visit(SubscriptFormattingRun* run) { visitImpl(run); }
    virtual void visit(TagReferenceRun* run) { visitImpl(run); }
    virtual void visit(ArticleReferenceRun* run) { visitImpl(run); }
    virtual void visit(IdRun* run) { visitImpl(run); }
    virtual void visit(TabRun* run) { visitImpl(run); }
    virtual void visit(ReferencePlaceholderRun* run) { visitImpl(run); }
    virtual void visit(LineBreakRun* run) { visitImpl(run); }
    virtual void visit(SoftLineBreakRun* run) { visitImpl(run); }
    virtual void visit(TableRun* run) { visitImpl(run); }
    virtual void visit(TableTag* run) { visitImpl(run); }
    virtual void visit(TableCellRun* run) { visitImpl(run); }

    virtual void visit(TableReferenceRun* run) {
        visitImpl(run);
        auto content = run->content();
        if (content) {
            TextRunVisitor::visit(content);
        }
    }

    virtual void visit(PictureReferenceRun* run) { visitImpl(run); }
    virtual void visit(WebReferenceRun* run) { visitImpl(run); }
    virtual void visit(InlineImageRun* run) { visitImpl(run); }
    virtual void visit(StickyRun* run) { visitImpl(run); }
};

using ResourceFiles = std::map<std::string, std::unique_ptr<std::istream>>;

bool dedupHeading(TextRun* heading, TextRun* article);

} // namespace duden
