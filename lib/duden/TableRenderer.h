#pragma once

#include "text/TextRun.h"
#include "text/Printers.h"
#include <functional>

namespace duden {

using SaveHtmlCallback = std::function<void(std::string const&, std::string const&)>;

class TableRenderer : TextRunVisitor {
    SaveHtmlCallback _saveHtml;
    RequestImageCallback _requestImage;
    int _id = 1;

    void visit(TableRun* run) override;
    void visit(TableReferenceRun* run) override;

public:
    TableRenderer(SaveHtmlCallback saveHtml, RequestImageCallback requestImage);
    void render(TextRun* run);
};

}
