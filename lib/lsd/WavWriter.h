#pragma once

#include <vector>

namespace dictlsd {

void createWav(std::vector<short> const& samples, std::vector<char>& wav);

}
