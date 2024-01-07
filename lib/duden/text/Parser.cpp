#include "Parser.h"
#include "Table.h"
#include <QtGui/QColor>
#include <regex>
#include <stack>
#include "common/bformat.h"

namespace duden {

namespace {

std::string findColorName(uint32_t rgb) {
    static std::map<uint32_t, std::string> map;
    if (map.empty()) {
        for (auto name : QColor::colorNames()) {
            auto color = QColor::fromString(name);
            map[color.rgb() & 0xffffff] = name.toStdString();
        }
    }
    auto it = map.find(rgb);
    if (it != end(map))
        return it->second;
    return "black";
}

struct TableInfo {
    TableRun* table = nullptr;
    int tableCellsParsed = 0;
    int tableRows = 0, tableColumns = 0;
    bool tableCellActive = false;
};

class Parser {
    ParsingContext* _context;
    const char* _ptr;

    std::string _plain;
    TextRun* _root = nullptr;
    TextRun* _current = nullptr;
    std::stack<TableInfo> _tableStack;

    TextRun* current() {
        if (!_current)
            throw std::runtime_error("parsing error, mismatched tags");
        return _current;
    }

    char peek() {
        if (*_ptr == 0)
            throw std::runtime_error("parsing error, unexpected end of file");
        return *_ptr;
    }

    bool chr(char& ch, bool acceptSemicolon, bool acceptClosingCurlyBrace = false) {
        ch = *_ptr;
        if (ch && ch != '@' && ch != '\\' &&
            (acceptSemicolon || ch != ';') &&
            (acceptClosingCurlyBrace || ch != '}')) {
            _ptr++;
            return true;
        }
        return false;
    }

    bool lit(const char* str) {
        auto p = _ptr;
        while (*str) {
            if (!*p)
                return false;
            if (*p++ != *str++)
                return false;
        }
        _ptr = p;
        return true;
    }

    void expect_lit(const char* str) {
        if (!lit(str)) {
            std::string actual;
            if (*_ptr == 0) {
                actual = "<eof>";
            } else {
                actual = *_ptr;
            }
            throw std::runtime_error(
                bformat("parsing error, expected \"%s\", but was \"%s\"", str, actual));
        }
    }

    void expect(bool result) {
        if (!result)
            throw std::runtime_error("parsing error");
    }

    bool digit(int& i) {
        if (*_ptr == '\0')
            return false;
        if ('0' <= peek() && peek() <= '9') {
            i = peek() - '0';
            _ptr++;
            return true;
        }
        return false;
    }

    bool dec(int64_t& i) {
        int d;
        if (!digit(d))
            return false;
        i = d;
        while (digit(d)) {
            i *= 10;
            i += d;
        }
        return true;
    }

    void sticky() {
        if (lit("C%ID=")) {
            int64_t i = 0;
            if (!dec(i)) {
                i = -1;
            }
            lit("%");
            current()->addRun(_context->make<IdRun>(i));
            return;
        }
        if (lit("C")) {
            while (*_ptr && *_ptr != '\n')
                ++_ptr;
            lit("\n");
            return;
        }

        int i;
        if (digit(i)) {
            current()->addRun(_context->make<StickyRun>(i));
        }

        // ignore unknown escape
    }

    void tname(std::string& name) {
        name.clear();
        while (peek() != '_' && peek() != '~' && peek() != '}') {
            name += peek();
            _ptr++;
        }
    }

    void scode(std::string& name) {
        name.clear();
        while (peek() != ':' && peek() != ';' && peek() != '}') {
            name += peek();
            _ptr++;
        }
    }

    bool sftag(std::string& name, bool& tilde) {
        if (!lit("F{_") && !lit("F{~"))
            return false;
        tilde = *(_ptr - 1) == '~';
        tname(name);
        expect_lit("}");
        return true;
    }

    bool eftag(std::string& name) {
        if (!lit("F{"))
            return false;
        tname(name);
        expect(lit("_}") || lit("~}"));
        return true;
    }

    ReferenceId sid() {
        std::string code;
        if (lit(".")) {
            scode(code);
        }

        int64_t i = -1, i2 = -1;
        if (lit(":")) {
            dec(i);
            if (lit("-")) {
                dec(i2);
            }
        }

        return {code, i, i2};
    }

    void sref() {
        auto placeholder = _context->make<ReferencePlaceholderRun>();
        push(placeholder);
        push(_context->make<TextRun>());
        text(false);
        finishPlain();
        pop();
        if (!lit(";")) { // misformed
            return;
        }
        auto id = sid();
        placeholder->setId(id);
        while (lit(";")) {
            text(false);
            finishPlain();
        }
        if (lit(":")) {
            int64_t from, to = 0;
            expect(dec(from));
            if (lit("-")) {
                expect(dec(to));
            }
            placeholder->setRange(from, to);
        }
        if (!lit("}")) {
            // misformed reference, the previous ';' wasn't properly escaped
            appendPlain(';');
            text(false);
            finishPlain();
            while (lit(";")) {
                text(false);
                finishPlain();
            }
            expect_lit("}");
            // TODO: might be possible to recover
            // and retry sid()
        }
        pop();
    }

    void wref() {
        push(_context->make<InlineSoundRun>());

        do {
            push(_context->make<TextRun>());
            text(false);
            finishPlain();
            pop();
        } while (lit(";"));

        expect_lit("}");
        pop();
    }

    void push(TextRun* run) {
        current()->addRun(run);
        _current = run;
    }

    void pop() {
        _current = current()->parent();
    }

    template <class T>
    void popIf() {
        if (dynamic_cast<T*>(current())) {
            pop();
        } else {
            // TODO: warn about mismatched tag
        }
    }

    TableInfo& table() {
        assertTable();
        return _tableStack.top();
    }

    void assertTable() {
        if (_tableStack.empty()) {
            throw std::runtime_error("table tag outside table");
        }
    }

    std::tuple<int, int> parseRange() {
        int64_t from, to;
        expect(dec(from));
        if (lit("-")) {
            expect(dec(to));
        } else {
            to = from;
        }
        return std::tuple<int, int>{from, to};
    }

    bool parseRgb(const std::string& text, uint32_t& rgb) {
        std::regex rx("^([0-9a-fA-F]{2})\\s?([0-9a-fA-F]{2})\\s?([0-9a-fA-F]{2})$");
        std::smatch m;
        if (!std::regex_match(text, m, rx))
            return false;
        auto r = std::stoul(m[1], nullptr, 16);
        auto g = std::stoul(m[2], nullptr, 16);
        auto b = std::stoul(m[3], nullptr, 16);
        rgb = (r << 16) | (g << 8) | b;
        return true;
    }

    void escape() {
        if (lit("\\")) {
            finishPlain();
            current()->addRun(_context->make<LineBreakRun>());
            return;
        }
        if (lit("{")) { // empty tag is an incorrect formatting
            appendPlain("\\{");
            text();
            lit("}");
            appendPlain("}");
            return;
        }
        if (lit("u{")) {
            finishPlain();
            push(_context->make<UnderlineFormattingRun>());
            text();
            finishPlain();
            pop();
            lit("}");
            return;
        }
        if (lit("b{")) {
            finishPlain();
            push(_context->make<BoldFormattingRun>());
            text();
            finishPlain();
            pop();
            lit("}");
            return;
        }
        if (lit("i{") || lit("s{")) {
            finishPlain();
            push(_context->make<ItalicFormattingRun>());
            text();
            finishPlain();
            pop();
            lit("}");
            return;
        }
        if (lit("sup{")) {
            finishPlain();
            push(_context->make<SuperscriptFormattingRun>());
            text();
            finishPlain();
            pop();
            lit("}");
            return;
        }
        if (lit("sub{")) {
            finishPlain();
            push(_context->make<SubscriptFormattingRun>());
            text();
            finishPlain();
            pop();
            lit("}");
            return;
        }
        if (lit("eb{")) {
            finishPlain();
            int64_t i;
            expect(dec(i));
            lit("}");
            current()->addRun(_context->make<TabRun>(i));
            return;
        }
        if (lit("ee")) {
            finishPlain();
            current()->addRun(_context->make<TabRun>(-1));
            return;
        }
        if (lit("tab{")) {
            finishPlain();
            TableInfo info;
            info.table = _context->make<TableRun>();
            _tableStack.push(info);
            push(info.table);
            text(true, true);
            return;
        }
        if (lit("tcn")) {
            assertTable();
            int64_t i;
            expect(dec(i));
            table().tableColumns = i;
            current()->addRun(_context->make<TableTag>(TableTagType::ColumnCount, i, -1));
            return;
        }
        if (lit("tln")) {
            assertTable();
            int64_t i;
            expect(dec(i));
            table().tableRows = i;
            current()->addRun(_context->make<TableTag>(TableTagType::RowCount, i, -1));
            return;
        }
        if (lit("tau")) {
            assertTable();
            current()->addRun(_context->make<TableTag>(TableTagType::tau));
            return;
        }
        if (lit("tcd")) {
            assertTable();
            current()->addRun(_context->make<TableTag>(TableTagType::tcd));
            return;
        }
        if (lit("tld")) {
            assertTable();
            current()->addRun(_context->make<TableTag>(TableTagType::tld));
            return;
        }
        if (lit("ter")) {
            assertTable();
            current()->addRun(_context->make<TableTag>(TableTagType::ter));
            return;
        }
        if (lit("ted")) {
            assertTable();
            current()->addRun(_context->make<TableTag>(TableTagType::ted));
            return;
        }
        if (lit("tfl")) {
            assertTable();
            auto [from, to] = parseRange();
            current()->addRun(_context->make<TableTag>(TableTagType::tfl, from, to));
            return;
        }
        if (lit("tcc")) {
            finishPlain();

            auto& info = table();
            if (info.tableCellsParsed >= info.tableRows * info.tableColumns) {
                if (lit("}")) {
                    if (info.tableCellActive) {
                        pop();
                    }
                    pop();
                    _tableStack.pop();
                }
            } else {
                if (info.tableCellActive) {
                    pop();
                }
                info.tableCellActive = true;
                push(_context->make<TableCellRun>());
                info.tableCellsParsed++;
            }

            return;
        }
        if (lit("tcl")) {
            assertTable();
            auto [from, to] = parseRange();
            current()->addRun(_context->make<TableTag>(TableTagType::tcl, from, to));
            return;
        }
        if (lit("tcr")) {
            assertTable();
            auto [from, to] = parseRange();
            current()->addRun(_context->make<TableTag>(TableTagType::tcr, from, to));
            return;
        }
        if (lit("tcm")) {
            assertTable();
            auto [from, to] = parseRange();
            current()->addRun(_context->make<TableTag>(TableTagType::tcm, from, to));
            return;
        }
        if (lit("tfu")) {
            assertTable();
            auto [from, to] = parseRange();
            current()->addRun(_context->make<TableTag>(TableTagType::tfu, from, to));
            return;
        }
        if (lit("tlt")) {
            assertTable();
            auto [from, to] = parseRange();
            current()->addRun(_context->make<TableTag>(TableTagType::tlt, from, to));
            return;
        }
        std::string name;
        bool tilde = false;
        if (sftag(name, tilde)) {
            finishPlain();
            uint32_t rgb;
            if (name == "ADD" || name == "UE") {
                push(_context->make<AddendumFormattingRun>());
            } else if (parseRgb(name, rgb)) {
                push(_context->make<ColorFormattingRun>(rgb, findColorName(rgb), tilde));
            } else if (name == "WebLink") {
                push(_context->make<WebLinkFormattingRun>());
            } else if (name == "Left" || name == "Right" || name == "Center") {
                push(_context->make<AlignmentFormattingRun>());
            } else {
                assert(false);
            }
            text();
            return;
        }
        if (eftag(name)) {
            finishPlain();
            if (name == "ADD" || name == "UE") {
                popIf<AddendumFormattingRun>();
            } else if (name == "WebLink") {
                popIf<WebLinkFormattingRun>();
            } else if (name == "Left" || name == "Right" || name == "Center") {
                popIf<AlignmentFormattingRun>();
            } else {
                popIf<ColorFormattingRun>();
            }
            return;
        }
        if (lit("S{")) {
            finishPlain();
            sref();
            return;
        }
        if (lit("w{")) {
            finishPlain();
            wref();
            return;
        }
        // TODO: log unknown escape
    }

    bool control() {
        if (lit("@")) {
            if (lit("@")) {
                appendPlain('@');
                return true;
            }
            if (lit("\\")) {
                appendPlain('\\');
                return true;
            }
            if (lit("~")) {
                appendPlain('~');
                return true;
            }
            if (lit(";")) {
                appendPlain(';');
                return true;
            }
            if (lit("S")) {
                appendPlain(u8"\u2191");
                return true;
            }
            finishPlain();
            sticky();
            return true;
        }
        if (lit("\\")) {
            if (lit("'")) {
                appendPlain("'");
                return true;
            }
            escape();
            return true;
        }
        return false;
    }

    template <class T>
    void appendPlain(T ch) {
        _plain += ch;
    }

    void finishPlain() {
        if (!_plain.empty()) {
            auto run = _context->make<PlainRun>(_plain);
            current()->addRun(run);
        }
        _plain.clear();
    }

    void text(bool acceptSemicolon = true, bool acceptClosingCurlyBrace = false) {
        for (;;) {
            if (*_ptr == '\0') {
                finishPlain();
                return;
            }
            if (control()) {
                continue;
            }
            if (lit("\n") || lit("\r\n")) {
                finishPlain();
                current()->addRun(_context->make<SoftLineBreakRun>());
                continue;
            }
            if (lit("~")) {
                appendPlain("\xC2\xA0");
                continue;
            }
            char ch;
            if (chr(ch, acceptSemicolon, acceptClosingCurlyBrace)) {
                appendPlain(ch);
            } else {
                break;
            }
        }
    }

public:
    Parser(ParsingContext* context, const std::string* text)
        : _context(context) {
        _ptr = text->data();
        _root = _context->make<TextRun>();
        _current = _root;
    }

    TextRun* parse() {
        text(true, true);
        if (*_ptr != '\0')
            throw std::runtime_error("incomplete parse");
        return _root;
    }
};

TextRun* stickyNumToRun(ParsingContext& context, int num) {
    switch (num) {
        case 1:
        case 4:
        case 7: return context.make<BoldFormattingRun>();
        case 3: return context.make<BoldItalicFormattingRun>();
        case 2: return context.make<ItalicFormattingRun>();
        default:
            return nullptr;
            break;
    }
}

void rewriteStickyFormatting(ParsingContext& context, TextRun* run) {
    auto& children = run->runs();
    for (auto child : children) {
        rewriteStickyFormatting(context, child);
    }

    auto findSticky = [&] (auto first) {
        return std::find_if(first, end(children), [](auto child) {
            return dynamic_cast<StickyRun*>(child);
        });
    };

    auto from = begin(children);
    for (;;) {
        auto first = findSticky(from);
        if (first == end(children))
            return;

        auto firstIndex = std::distance(begin(children), first);
        auto next = findSticky(first + 1);
        auto num = static_cast<StickyRun*>(*first)->num();
        auto container = stickyNumToRun(context, num);

        if (container) {
            for (auto it = first + 1; it != next; ++it) {
                container->addRun(*it);
            }

            children.erase(first, next);
            from = children.insert(begin(children) + firstIndex, container) + 1;
            container->setParent(run);
        } else {
            children.erase(first);
            from = begin(children) + firstIndex;
        }
    }
}

class BoldItalicRewriter : public TextRunVisitor {
    ParsingContext* _context;

public:
    BoldItalicRewriter(ParsingContext* context) : _context(context) { }

    void visit(BoldItalicFormattingRun* run) override {
        auto bold = _context->make<BoldFormattingRun>();
        auto italic = _context->make<ItalicFormattingRun>();
        bold->addRun(italic);
        for (auto child : run->runs()) {
            italic->addRun(child);
        }
        run->parent()->replace(run, bold);
        TextRunVisitor::visit(run);
    }
};

class TableVisitor : public TextRunVisitor {
    ParsingContext* _context;

public:
    TableVisitor(ParsingContext* context) : _context(context) {}

    void visit(TableRun* run) override {
        visitImpl(run);
        run->setTable(std::make_unique<Table>(run, _context));
    }
};

void rewriteParsedRun(ParsingContext& context, TextRun* run) {
    rewriteStickyFormatting(context, run);
    BoldItalicRewriter boldItalicRewriter(&context);
    TableVisitor tableVisitor(&context);
    run->accept(&boldItalicRewriter);
    run->accept(&tableVisitor);
}

}

TextRun* parseDudenText(ParsingContext& context, std::string const& text) {
    Parser parser(&context, &text);
    auto run = parser.parse();
    rewriteParsedRun(context, run);
    return run;
}

} // namespace duden
