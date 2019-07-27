#include "Table.h"

namespace duden {

void Table::visit(TableTag* run) {
    switch (run->type()) {
        case TableTagType::ColumnCount: _columns = run->from(); break;
        case TableTagType::RowCount: _rows = run->from(); break;
        case TableTagType::ter: _hmerged.back() = true; break;
        case TableTagType::ted: _vmerged.back() = true; break;
        default: break;
    }
}

int Table::index(int row, int column) const {
    return _columns * row + column;
}

void Table::fixSpans() {
    for (auto r = 0; r < _rows; ++r) {
        int last = 0;
        for (auto c = 0; c < _columns; ++c) {
            if (_hmerged.at(index(r, c))) {
                _hspans.at(index(r, last))++;
            } else {
                last = c;
            }
        }
    }

    for (auto c = 0; c < _columns; ++c) {
        int last = 0;
        for (auto r = 0; r < _rows; ++r) {
            if (_vmerged.at(index(r, c))) {
                _vspans.at(index(last, c))++;
            } else {
                last = r;
            }
        }
    }
}

void Table::visit(TableCellRun* run) {
    _cells.push_back(run);
    _hspans.push_back(1);
    _vspans.push_back(1);
    _hmerged.push_back(false);
    _vmerged.push_back(false);
    TextRunVisitor::visit(run);
}

Table::Table(TableRun* table, ParsingContext* context) : _context(context) {
    table->accept(this);
    fixSpans();
    assert(_cells.size() == _hspans.size());
    assert(_cells.size() == _vspans.size());
    assert(_cells.size() == _hmerged.size());
    assert(_cells.size() == _vmerged.size());
}

int Table::rows() const {
    return _rows;
}

int Table::columns() const {
    return _columns;
}

TextRun* Table::cell(int row, int column) {
    return _cells.at(index(row, column));
}

int Table::hspan(int row, int column) const {
    return _hspans.at(index(row, column));
}

int Table::vspan(int row, int column) const {
    return _vspans.at(index(row, column));
}

int Table::merged(int row, int column) const {
    return _hmerged.at(index(row, column)) || _vmerged.at(index(row, column));
}

} // namespace duden
