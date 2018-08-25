#pragma once

#include <string>

#ifdef __MINGW32__
#include <windows.h>
#else
#include <fstream>
#endif

class UnicodePathFile {
#ifdef __MINGW32__
    HANDLE _file = NULL;
#else
    std::fstream _file;
#endif

public:
    UnicodePathFile(const UnicodePathFile&) = delete;
    UnicodePathFile& operator=(const UnicodePathFile&) = delete;
    UnicodePathFile(std::string path, bool write);
    void write(const char* buf, size_t len);
    size_t read(char* buf, size_t len);
    void seek(unsigned pos);
    size_t tell();
    ~UnicodePathFile();
};
