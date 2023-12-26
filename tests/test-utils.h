#pragma once

#include "lib/common/version.h"
#include "lib/common/bformat.h"
#include "lib/common/Log.h"

#include <filesystem>
#include <vector>
#include <stdint.h>
#include <stdio.h>

class TestLog : public Log {
    void reportLog(std::string, bool) override { }
    void reportProgress(int) override { }
    void reportProgressReset(std::string) override { }
};

inline std::vector<uint8_t> read_all_bytes(std::filesystem::path path) {
    auto f = fopen(path.u8string().c_str(), "rb");
    if (!f)
        throw std::runtime_error("can't open file");
    fseek(f, 0, SEEK_END);
    auto filesize = ftell(f);
    std::vector<uint8_t> res(filesize);
    fseek(f, 0, SEEK_SET);
    fread(&res[0], 1, res.size(), f);
    fclose(f);
    return res;
}

inline std::string read_all_text(std::filesystem::path path) {
    auto vec = read_all_bytes(path);
    return {
        reinterpret_cast<char*>(&vec[0]),
        reinterpret_cast<char*>(&vec[vec.size()])
    };
}

inline std::filesystem::path testPath(const char* relative) {
    return std::filesystem::u8path(bformat("%s/tests/%s", g_sourceDir, relative));
}
