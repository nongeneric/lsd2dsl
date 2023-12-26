#include "TableRenderer.h"

#include "HtmlRenderer.h"
#include "lib/common/bformat.h"

namespace duden {

TableRenderer::TableRenderer(SaveHtmlCallback saveHtml,
                             RequestImageCallback requestImage)
    : _saveHtml(saveHtml), _requestImage(requestImage) {}

void TableRenderer::render(TextRun* run) {
    run->accept(this);
}

void TableRenderer::visit(TableRun* run) {
    auto name = bformat("rendered_table_%04d.png", _id++);
    _saveHtml(name, printHtml(run, _requestImage));
    run->setRenderedName(name);
}

void TableRenderer::visit(TableReferenceRun* run) {
    TextRunVisitor::visit(run->content());
}

} // namespace duden
