#pragma once

#include "TextRun.h"
#include "duden/LdFile.h"
#include <functional>

namespace duden {

using ResolveArticle = std::function<std::string(int64_t)>;

void resolveReferences(ParsingContext& context, TextRun* run, LdFile const& ld);
void resolveArticleReferences(TextRun* run, ResolveArticle resolveArticle);
void inlineReferences(ParsingContext& context, TextRun* run, const ResourceFiles& files);

} // namespace duden
