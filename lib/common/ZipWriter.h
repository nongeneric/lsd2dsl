#pragma once

#include <filesystem>
#include <string>
#include <vector>

class ZipWriter {
    void* _zip = nullptr;
    std::filesystem::path _path;

public:
    explicit ZipWriter(std::filesystem::path path);
    void addFile(std::string name, const void* ptr, unsigned size);
    ~ZipWriter();
};
