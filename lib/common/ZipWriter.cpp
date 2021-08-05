#include "ZipWriter.h"

#include <zlib.h>
#include <minizip/mz_zip.h>
#include <minizip/mz_compat.h>
#include <stdexcept>
#include <cstring>
#include <time.h>

inline constexpr int UNICODE_BIT = 1 << 11;
inline constexpr int ZIP_VERSION = 36;

ZipWriter::ZipWriter(std::string path) : _path(path) { }

void ZipWriter::addFile(std::string name, const void* ptr, unsigned size) {
    if (!_zip) {
        _zip = zipOpen64(_path.c_str(), false);
        if (!_zip)
            throw std::runtime_error("can't create zip file");
    }

    zip_fileinfo file_info;
    memset(&file_info, 0, sizeof(file_info));
    file_info.dosDate = mz_zip_time_t_to_dos_date(time(nullptr));

    auto ret = zipOpenNewFileInZip4_64(_zip,
                                       name.c_str(),
                                       &file_info,
                                       nullptr,
                                       0,
                                       nullptr,
                                       0,
                                       nullptr,
                                       Z_DEFLATED,
                                       MZ_COMPRESS_LEVEL_DEFAULT,
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
    if (_zip) {
        zipClose(_zip, nullptr);
    }
}
