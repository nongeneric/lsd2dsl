#pragma once

#include "TextRun.h"
#include <functional>

namespace duden {

using RequestImageCallback = std::function<std::vector<char>(std::string)>;

std::string printDsl(TextRun* run);
std::string printHtml(TextRun* run, RequestImageCallback requestImage = {});
std::string printTree(TextRun* run);

} // namespace duden
