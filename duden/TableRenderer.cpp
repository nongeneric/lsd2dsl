#include "TableRenderer.h"
#include "HtmlRenderer.h"
#include "tools/bformat.h"

namespace duden {

TableRenderer::TableRenderer(SetImageCallback setImage, RequestImageCallback requestImage)
    : _setImage(setImage), _requestImage(requestImage) {}

void TableRenderer::render(TextRun* run) {
    run->accept(this);
}

void TableRenderer::visit(TableRun* run) {
    auto html = printHtml(run, _requestImage);
    auto vec = renderHtml(html);
    auto name = bformat("rendered_table_%04d.bmp", _id++);
    run->setRenderedName(name);
    _setImage(vec, name);
}

void TableRenderer::visit(TableReferenceRun* run) {
    TextRunVisitor::visit(run->content());
}

} // namespace duden
