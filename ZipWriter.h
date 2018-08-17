#pragma once

#include <string>
#include <vector>

class ZipWriter {
    void* _zip;

public:
    ZipWriter(std::string path);
    void addFile(std::string name, const void* ptr, unsigned size);
    ~ZipWriter();
};
