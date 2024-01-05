#include "AdpDecoder.h"
#include <boost/algorithm/string.hpp>
#include <array>
#include <cmath>
#include <algorithm>
#include <filesystem>

namespace {

std::array<float, 49> stepTable {
    16,  17,  19,  21,  23,  25,   28,   31,   34,   37,  41,  45,  50,
    55,  60,  66,  73,  80,  88,   97,   107,  118,  130, 143, 157, 173,
    190, 209, 230, 253, 279, 307,  337,  371,  408,  449, 494, 544, 598,
    658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552
};

std::array<int, 8> valueTable {
    -1, -1, -1, -1, 2, 4, 6, 8
};

}

void duden::decodeAdp(const std::vector<char>& input, std::vector<int16_t>& samples) {
    samples.resize(input.size() * 2);
    auto outptr = samples.data();
    double sample = 0;
    int index = 0;
    for (uint8_t byte : input) {
        for (auto code : {byte >> 4, byte & 0xf}) {
            auto b0 = code & 1;
            auto b1 = (code >> 1) & 1;
            auto b2 = (code >> 2) & 1;
            auto ss = stepTable[index];
            auto d = ss * b2 + ss / 2 * b1 + ss / 4 * b0 + ss / 8;
            sample += code & 8 ? -d : d;
            *outptr++ = static_cast<int16_t>(std::round(std::clamp(sample, -32768., 32767.)));
            index = std::clamp(index + valueTable[code & 7], 0, 48);
        }
    }
}

bool duden::replaceAdpExtWithWav(std::string& name) {
    auto asPath = std::filesystem::u8path(name);
    auto ext = boost::to_lower_copy(asPath.extension().u8string());
    if (ext == ".adp") {
        asPath.replace_extension(".wav");
        name = asPath.u8string();
        return true;
    }
    return false;
}
