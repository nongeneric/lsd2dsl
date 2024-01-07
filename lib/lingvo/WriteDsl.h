#pragma once

#include "lsd.h"
#include "lingvo/lsd.h"
#include "common/Log.h"

namespace lingvo {

void writeDSL(const lingvo::LSDDictionary* reader,
              std::filesystem::path lsdName,
              std::filesystem::path outputPath,
              bool dumb,
              Log& log);

}
