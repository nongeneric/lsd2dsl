#pragma once

#include "text/TextRun.h"
#include "text/Printers.h"
#include <functional>

namespace duden {

using SetImageCallback = std::function<void(const std::vector<uint8_t>&, std::string)>;

class TableRenderer : TextRunVisitor {
    SetImageCallback _setImage;
    RequestImageCallback _requestImage;
    int _id = 1;

    void visit(TableRun* run) override;
    void visit(TableReferenceRun* run) override;

public:
    TableRenderer(SetImageCallback setImage, RequestImageCallback requestImage);
    void render(TextRun* run);
};

}
