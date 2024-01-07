#include "ZipWriter.h"

#include "minizip/zip.h"
#include <fmt/format.h>
#include <zlib.h>

#include <fstream>
#include <stdexcept>
#include <cstring>
#include <time.h>

inline constexpr int UNICODE_BIT = 1 << 11;
inline constexpr int ZIP_VERSION = 36;

zlib_filefunc64_def makeZipFileFunc() {
    zlib_filefunc64_def filefunc{};
    static auto tofstr = [] (voidpf stream) { return reinterpret_cast<std::ofstream*>(stream); };
    filefunc.zopen64_file = [](voidpf, const void* filename, int mode) -> void* {
        if (!(mode & ZLIB_FILEFUNC_MODE_CREATE))
            throw std::runtime_error("reading zip files is not implemented");
        auto typedPath = static_cast<const char*>(filename);
        auto fstr = new std::ofstream(
            std::filesystem::u8path(typedPath),
            std::ios::binary);
        if (fstr->fail())
            throw std::runtime_error(fmt::format("can't open file for writing: {}", typedPath));
        return fstr;
    };
    filefunc.zread_file = [] (voidpf, voidpf, void*, uLong) -> uLong {
        throw std::runtime_error("unexpected zip read");
    };
    filefunc.zwrite_file = [] (voidpf, voidpf stream, const void* buf, uLong size) -> uLong {
        tofstr(stream)->write(static_cast<const char*>(buf), size);
        return size;
    };
    filefunc.ztell64_file = [] (voidpf, voidpf stream) -> ZPOS64_T {
        return tofstr(stream)->tellp();
    };
    filefunc.zseek64_file = [] (voidpf, voidpf stream, ZPOS64_T offset, int zipOrigin) -> long {
        auto origin = zipOrigin == ZLIB_FILEFUNC_SEEK_CUR ? std::ios_base::cur
                    : zipOrigin == ZLIB_FILEFUNC_SEEK_END ? std::ios_base::end
                    : std::ios_base::beg;
        if (!tofstr(stream)->seekp(offset, origin))
            return -1;
        return 0;
    };
    filefunc.zclose_file = [] (voidpf, voidpf stream) -> int {
        delete tofstr(stream);
        return 0;
    };
    filefunc.zerror_file = [] (voidpf, voidpf stream) -> int {
        return tofstr(stream)->fail() ? 1 : 0;
    };
    return filefunc;
}

ZipWriter::ZipWriter(std::filesystem::path path) : _path(path) { }

void ZipWriter::addFile(std::string name, const void* ptr, unsigned size) {
    if (!_zip) {
        auto fileFunc = makeZipFileFunc();
        _zip = zipOpen2_64(_path.u8string().c_str(), false, NULL, &fileFunc);
        if (!_zip)
            throw std::runtime_error(fmt::format("can't create file:", name));
    }

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
    if (_zip) {
        zipClose(_zip, nullptr);
    }
}
