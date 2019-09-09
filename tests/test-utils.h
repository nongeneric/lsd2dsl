#pragma once

#include <vector>
#include <stdint.h>
#include <stdio.h>

inline std::vector<uint8_t> read_all_bytes(const char* path) {
    auto f = fopen(path, "rb");
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

inline std::string read_all_text(const char* path) {
    auto vec = read_all_bytes(path);
    return {
        reinterpret_cast<char*>(&vec[0]),
        reinterpret_cast<char*>(&vec[vec.size()])
    };
}
