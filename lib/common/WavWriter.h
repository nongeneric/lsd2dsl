#pragma once

#include <vector>
#include <stdint.h>

namespace common {

void createWav(std::vector<int16_t> const& samples, std::vector<char>& wav, int rate, int channels);

}
