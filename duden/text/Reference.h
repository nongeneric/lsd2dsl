#pragma once

#include "TextRun.h"
#include "duden/LdFile.h"

namespace duden {

void resolveReferences(ParsingContext& context, TextRun* run, LdFile const& ld);
void inlineReferences(ParsingContext& context, TextRun* run, ResourceFiles& files);

} // namespace duden
