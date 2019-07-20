#pragma once

#include "text/TextRun.h"
#include <functional>

namespace duden {

using SetImageCallback = std::function<void(const std::vector<uint8_t>&, std::string)>;

class TableRenderer : TextRunVisitor {
    SetImageCallback _callback;
    int _id = 1;

    void visit(TableRun* run) override;
    void visit(TableReferenceRun* run) override;

public:
    TableRenderer(SetImageCallback callback);
    void render(TextRun* run);
};
}
