#pragma once

#include <string>
#include <vector>


struct zip;
class ZipWriter {
    zip* _zip;
    std::vector<std::vector<char>> _buffers;
    std::string _path;
    unsigned _memoryInBuffers;
    void flushZip();
public:
    ZipWriter(std::string path);
    void addFile(std::string name, void* ptr, unsigned size);
    void close();
    ~ZipWriter();
};
