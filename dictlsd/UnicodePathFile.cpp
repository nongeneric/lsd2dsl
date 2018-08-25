#include "UnicodePathFile.h"
#include "tools.h"

using namespace dictlsd;

UnicodePathFile::UnicodePathFile(std::string path, bool write) {
#ifdef __MINGW32__
    auto utf16path = toUtf16(path);
    _file = CreateFileW((LPCWSTR)utf16path.data(),
                        write ? GENERIC_WRITE : GENERIC_READ,
                        write ? 0 : FILE_SHARE_READ,
                        NULL,
                        write ? CREATE_ALWAYS : OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (_file == INVALID_HANDLE_VALUE)
        throw std::runtime_error("can't open file " + path);
#else
    _file = std::fstream(path, write ? std::ios_base::out : std::ios_base::in);
    if (!_file.is_open())
        throw std::runtime_error("can't open file " + path);
#endif
}

void UnicodePathFile::write(const char *buf, size_t len) {
#ifdef __MINGW32__
    DWORD written;
    auto code = WriteFile(_file, buf, len, &written, NULL);
    if (!code) {
        throw std::runtime_error("can't write to file");
    }
#else
    _file.write(buf, len);
#endif
}

size_t UnicodePathFile::read(char *buf, size_t len) {
#ifdef __MINGW32__
    DWORD read;
    auto code = ReadFile(_file, buf, len, &read, NULL);
    if (!code) {
        throw std::runtime_error("can't read file");
    }
    return read;
#else
    _file.read(buf, len);
    return _file.gcount();
#endif
}

void UnicodePathFile::seek(unsigned pos) {
#ifdef __MINGW32__
    SetFilePointer(_file, pos, NULL, FILE_BEGIN);
#else
    _file.seekg(pos);
#endif
}

size_t UnicodePathFile::tell() {
#ifdef __MINGW32__
    return SetFilePointer(_file, 0, NULL, FILE_CURRENT);
#else
    return _file.tellg();
#endif
}

UnicodePathFile::~UnicodePathFile() {
#ifdef __MINGW32__
    CloseHandle(_file);
#endif
}
