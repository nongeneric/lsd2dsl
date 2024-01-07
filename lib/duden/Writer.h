#pragma once

#include "Dictionary.h"
#include "InfFile.h"
#include "duden/text/Reference.h"
#include "common/Log.h"
#include <functional>

namespace duden {

enum class LogLevel {
    Regular,
    Verbose
};

void writeDSL(std::filesystem::path infPath,
              std::filesystem::path outputPath,
              int index,
              Log& progress);

std::string defaultArticleResolve(const std::map<int32_t, HeadingGroup>& groups,
                                  int64_t offset,
                                  std::string hint,
                                  ParsingContext& context);
}
