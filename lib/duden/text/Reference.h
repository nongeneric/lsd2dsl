#pragma once

#include "TextRun.h"
#include "lib/duden/LdFile.h"
#include "lib/duden/IFileSystem.h"
#include <functional>

namespace duden {

using ResolveArticle = std::function<std::string(int64_t)>;

void resolveReferences(ParsingContext& context,
                       TextRun* run,
                       LdFile const& ld,
                       IFileSystem* filesystem);
void resolveArticleReferences(TextRun* run, ResolveArticle resolveArticle);
void inlineReferences(ParsingContext& context, TextRun* run, const ResourceFiles& files);

} // namespace duden
