#pragma once

#include "TextRun.h"

namespace duden {

std::string printDsl(TextRun* run);
std::string printHtml(TextRun* run);
std::string printTree(TextRun* run);

} // namespace duden
