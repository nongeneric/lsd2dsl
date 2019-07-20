#include "TableRenderer.h"
#include "HtmlRenderer.h"
#include "tools/bformat.h"
#include "text/Printers.h"

namespace duden {

TableRenderer::TableRenderer(SetImageCallback callback) : _callback(callback) {}

void TableRenderer::render(TextRun* run) {
    run->accept(this);
}

void TableRenderer::visit(TableRun* run) {
    auto html = printHtml(run);
    auto vec = renderHtml(html);
    auto name = bformat("rendered_table_%04d.bmp", _id++);
    run->setRenderedName(name);
    _callback(vec, name);
}

void TableRenderer::visit(TableReferenceRun* run) {
    TextRunVisitor::visit(run->content());
}

} // namespace duden
