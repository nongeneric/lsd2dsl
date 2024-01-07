#pragma once

#include "common/Log.h"

#include <QImage>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace duden {

void renderHtml(std::function<void(QImage const&)> handler,
                std::vector<std::string const*> htmls,
                Log& log,
                unsigned batchSize = 1000);

std::vector<uint8_t> qImageToPng(QImage const& image);

} // namespace duden
