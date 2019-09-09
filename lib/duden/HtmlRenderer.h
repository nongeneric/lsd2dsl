#pragma once

#include <vector>
#include <string>
#include <stdint.h>

namespace duden {

std::vector<uint8_t> renderHtml(std::string const& html);

} // namespace duden
