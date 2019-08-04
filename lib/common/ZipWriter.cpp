#include "ZipWriter.h"

#include <minizip/zip.h>
#include <stdexcept>
#include <cstring>
#include <time.h>

inline constexpr int UNICODE_BIT = 1 << 11;
inline constexpr int ZIP_VERSION = 36;

ZipWriter::ZipWriter(std::string path) {
    _zip = zipOpen64(path.c_str(), false);
    if (!_zip)
        throw std::runtime_error("can't create zip file");
}

void ZipWriter::addFile(std::string name, const void* ptr, unsigned size) {
    auto t = time(nullptr);
    auto tm = *localtime(&t);
    zip_fileinfo info{{(unsigned)tm.tm_sec,
                       (unsigned)tm.tm_min,
                       (unsigned)tm.tm_hour,
                       (unsigned)tm.tm_mday,
                       (unsigned)tm.tm_mon,
                       (unsigned)tm.tm_year + 1900},
                      0,
                      0,
                      0};
    auto ret = zipOpenNewFileInZip4_64(_zip,
                                       name.c_str(),
                                       &info,
                                       nullptr,
                                       0,
                                       nullptr,
                                       0,
                                       nullptr,
                                       Z_DEFLATED,
                                       Z_DEFAULT_COMPRESSION,
                                       0,
                                       -MAX_WBITS,
                                       DEF_MEM_LEVEL,
                                       Z_DEFAULT_STRATEGY,
                                       NULL,
                                       0,
                                       ZIP_VERSION,
                                       UNICODE_BIT,
                                       1);
    if (ret)
        throw std::runtime_error("can't add a new file to zip");
    ret = zipWriteInFileInZip(_zip, ptr, size);
    if (ret)
        throw std::runtime_error("can't write to zip");
    ret = zipCloseFileInZip(_zip);
    if (ret)
        throw std::runtime_error("can't save zip");
}

ZipWriter::~ZipWriter() {
    zipClose(_zip, nullptr);
}
