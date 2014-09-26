#pragma once

#include "dictlsd/lsd.h"
#include <string>
#include <functional>

void writeDSL(const dictlsd::LSDDictionary* reader, std::string lsdName, std::string outputPath, std::function<void(int,std::string)> log);
