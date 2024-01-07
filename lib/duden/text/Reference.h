#pragma once

#include "TextRun.h"
#include "duden/LdFile.h"
#include "duden/IFileSystem.h"
#include <functional>

namespace duden {

using ResolveArticle = std::function<std::string(int64_t, const std::string&)>;

void resolveReferences(ParsingContext& context,
                       TextRun* run,
                       LdFile const& ld,
                       IFileSystem* filesystem);
void resolveArticleReferences(TextRun* run, ResolveArticle resolveArticle);
void inlineReferences(ParsingContext& context, TextRun* run, const ResourceFiles& files);
std::string trimReferenceDisplayName(std::string str);

} // namespace duden
