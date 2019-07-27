#pragma once

#include "Dictionary.h"
#include "InfFile.h"
#include "lib/common/Log.h"
#include "lib/common/filesystem.h"
#include <functional>

namespace duden {

enum class LogLevel {
    Regular,
    Verbose
};

void writeDSL(fs::path infPath,
              fs::path outputPath,
              Log& progress);

}
