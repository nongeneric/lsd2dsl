#pragma once

#include "Dictionary.h"
#include "InfFile.h"
#include "tools/filesystem.h"
#include <functional>

namespace duden {

enum class LogLevel {
    Regular,
    Verbose
};

using ProgressCallback = std::function<void(int, LogLevel, std::string)>;

void writeDSL(fs::path inputDir,
              Dictionary& dict,
              InfFile const& inf,
              fs::path outputPath,
              ProgressCallback progress);

}
