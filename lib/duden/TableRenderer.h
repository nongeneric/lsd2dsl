#pragma once

#include "text/TextRun.h"
#include "text/Printers.h"
#include <functional>
#include <unordered_map>

namespace duden {

class TableRenderer : TextRunVisitor {
    RequestImageCallback _requestImage;
    int _id = 1;
    std::unordered_map<std::string, std::string> _htmlToNameMap;

    void visit(TableRun* run) override;
    void visit(TableReferenceRun* run) override;

public:
    TableRenderer(RequestImageCallback requestImage);
    void render(TextRun* run);
    auto const& getHtmls() const { return _htmlToNameMap; }
};

}
