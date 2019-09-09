#pragma once

#include "TextRun.h"

namespace duden {

enum class TableHorizontalAlignment { Left, Right };

class Table : TextRunVisitor {
    ParsingContext* _context;
    int _columns, _rows;
    std::vector<TextRun*> _cells;
    std::vector<int> _hspans;
    std::vector<int> _vspans;
    std::vector<bool> _hmerged;
    std::vector<bool> _vmerged;
    int _lastNonEmptyCell = 0;

    void visit(TableTag* run) override;
    int index(int row, int column) const;
    void fixSpans();
    void visit(TableCellRun* run) override;

public:
    Table(TableRun* table, ParsingContext* context);
    int rows() const;
    int columns() const;
    TextRun* cell(int row, int column);
    TableHorizontalAlignment alignment(int row, int column);
    int hspan(int row, int column) const;
    int vspan(int row, int column) const;
    int merged(int row, int column) const;
};

} // namespace duden
