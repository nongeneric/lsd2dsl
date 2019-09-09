#pragma once

#include "TextRun.h"

namespace duden {

TextRun* parseDudenText(ParsingContext& context, std::string const& text);

}
