#include "ZipWriter.h"

#define ZIP_EXTERN __attribute__ ((visibility ("default")))

#include <zip.h>
#include <stdexcept>
#include <cstring>

unsigned const BUFFERS_LIMIT = 200 * 1024 * 1024;

ZipWriter::ZipWriter(std::string path)
    : _path(path), _memoryInBuffers(0) {
    _zip = zip_open(_path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, NULL);
    if (!_zip) {
        throw std::runtime_error("can't create zip file");
    }
}

void ZipWriter::flushZip() {
    zip_close(_zip);
    _zip = zip_open(_path.c_str(), 0, NULL);
    if (!_zip) {
        throw std::runtime_error("error reopening zip file after flush");
    }
}

void ZipWriter::addFile(std::string name, void *ptr, unsigned size) {
    if (_memoryInBuffers + size > BUFFERS_LIMIT) {
        flushZip();
        _memoryInBuffers = 0;
        _buffers.clear();
    }
    _memoryInBuffers += size;
    std::vector<char> buf;
    buf.resize(size);
    memcpy(&buf[0], ptr, size);
    _buffers.push_back(std::move(buf));
    zip_source* source = zip_source_buffer(
                _zip, _buffers.back().data(), _buffers.back().size(), 0);
    zip_file_add(_zip, name.c_str(), source, 0);
}

void ZipWriter::close() {
    zip_close(_zip);
    _zip = 0;
}

ZipWriter::~ZipWriter() {
    zip_close(_zip);
}
