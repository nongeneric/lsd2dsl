#pragma once

#include <vector>
#include <string>
#include <stdint.h>

namespace duden {

inline constexpr int ADP_SAMPLE_RATE = 21000;

void decodeAdp(const std::vector<char>& input, int16_t* output);
bool replaceAdpExtWithWav(std::string& name);

}

