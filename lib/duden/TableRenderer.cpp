#include "TableRenderer.h"

#include "HtmlRenderer.h"
#include <fmt/format.h>

namespace duden {

TableRenderer::TableRenderer(RequestImageCallback requestImage)
    : _requestImage(requestImage) {}

void TableRenderer::render(TextRun* run) {
    run->accept(this);
}

void TableRenderer::visit(TableRun* run) {
    auto html = printHtml(run, _requestImage);
    if (auto it = _htmlToNameMap.find(html); it != end(_htmlToNameMap)) {
        run->setRenderedName(it->second);
        return;
    }
    auto name = fmt::format("rendered_table_{:04d}.png", _id++);
    run->setRenderedName(name);
    _htmlToNameMap.emplace(std::move(html), std::move(name));
}

void TableRenderer::visit(TableReferenceRun* run) {
    TextRunVisitor::visit(run->content());
}

} // namespace duden
