#include "common/BitStream.h"

#include "common/CommonTools.h"

#include <bitset>
#include <stdexcept>
#include <assert.h>
#include <stdint.h>
#include <functional>
#include <cstring>

namespace common {

bool advance(unsigned& bitPos) {
    bitPos = (bitPos + 1) % 8;
    return bitPos == 0;
}

unsigned BitStreamAdapter::readBit() {
    if (_bitPos == 0) {
        readSome(&_cache, 1);
    }
    std::bitset<8> bset(_cache);
    unsigned bit = bset[7 - _bitPos];
    advance(_bitPos);
    return bit;
}

BitStreamAdapter::BitStreamAdapter(IRandomAccessStream* ras)
    : _ras(ras), _bitPos(0) { }

unsigned BitStreamAdapter::read(unsigned count) {
    assert(count <= sizeof(unsigned) * 8);
    unsigned res = 0;
    while(count) {
        res <<= 1;
        res |= readBit();
        count--;
    }
    return res;
}

unsigned BitStreamAdapter::readSome(void *dest, unsigned byteCount) {
    return _ras->readSome(dest, byteCount);
}

void BitStreamAdapter::seek(unsigned pos) {
    _ras->seek(pos);
    _bitPos = 0;
}

void BitStreamAdapter::toNearestByte() {
    _bitPos = 0;
}

unsigned BitStreamAdapter::tell() {
    return _ras->tell();
}

InMemoryStream::InMemoryStream(const void *buf, unsigned size)
    : _buf((const uint8_t*)buf), _size(size), _pos(0) { }

unsigned InMemoryStream::readSome(void *dest, unsigned byteCount) {
    byteCount = std::min(byteCount, _size - _pos);
    memcpy(dest, _buf + _pos, byteCount);
    _pos += byteCount;
    return byteCount;
}

void InMemoryStream::seek(unsigned pos) {
    assert(pos <= _size);
    _pos = pos;
}

unsigned InMemoryStream::tell() {
    return _pos;
}

IRandomAccessStream::~IRandomAccessStream() { }
IBitStream::~IBitStream() { }

unsigned char xor_pad[256] = {
    0x9C, 0xDF, 0x9B, 0xF3, 0xBE, 0x3A, 0x83, 0xD8,
    0xC9, 0xF5, 0x50, 0x98, 0x35, 0x4E, 0x7F, 0xBB,
    0x89, 0xC7, 0xE9, 0x6B, 0xC4, 0xC8, 0x4F, 0x85,
    0x1A, 0x10, 0x43, 0x66, 0x65, 0x57, 0x55, 0x54,
    0xB4, 0xFF, 0xD7, 0x17, 0x06, 0x31, 0xAC, 0x4B,
    0x42, 0x53, 0x5A, 0x46, 0xC5, 0xF8, 0xCA, 0x5E,
    0x18, 0x38, 0x5D, 0x91, 0xAA, 0xA5, 0x58, 0x23,
    0x67, 0xBF, 0x30, 0x3C, 0x8C, 0xCF, 0xD5, 0xA8,
    0x20, 0xEE, 0x0B, 0x8E, 0xA6, 0x5B, 0x49, 0x3F,
    0xC0, 0xF4, 0x13, 0x80, 0xCB, 0x7B, 0xA7, 0x1D,
    0x81, 0x8B, 0x01, 0xDD, 0xE3, 0x4C, 0x9A, 0xCE,
    0x40, 0x72, 0xDE, 0x0F, 0x26, 0xBD, 0x3B, 0xA3,
    0x05, 0x37, 0xE1, 0x5F, 0x9D, 0x1E, 0xCD, 0x69,
    0x6E, 0xAB, 0x6D, 0x6C, 0xC3, 0x71, 0x1F, 0xA9,
    0x84, 0x63, 0x45, 0x76, 0x25, 0x70, 0xD6, 0x8F,
    0xFD, 0x04, 0x2E, 0x2A, 0x22, 0xF0, 0xB8, 0xF2,
    0xB6, 0xD0, 0xDA, 0x62, 0x75, 0xB7, 0x77, 0x34,
    0xA2, 0x41, 0xB9, 0xB1, 0x74, 0xE4, 0x95, 0x1B,
    0x3E, 0xE7, 0x00, 0xBC, 0x93, 0x7A, 0xE8, 0x86,
    0x59, 0xA0, 0x92, 0x11, 0xF7, 0xFE, 0x03, 0x2F,
    0x28, 0xFA, 0x27, 0x02, 0xE5, 0x39, 0x21, 0x96,
    0x33, 0xD1, 0xB2, 0x7C, 0xB3, 0x73, 0xC6, 0xE6,
    0xA1, 0x52, 0xFB, 0xD4, 0x9E, 0xB0, 0xE2, 0x16,
    0x97, 0x08, 0xF6, 0x4A, 0x78, 0x29, 0x14, 0x12,
    0x4D, 0xC1, 0x99, 0xBA, 0x0D, 0x3D, 0xEF, 0x19,
    0xAF, 0xF9, 0x6F, 0x0A, 0x6A, 0x47, 0x36, 0x82,
    0x07, 0x9F, 0x7D, 0xA4, 0xEA, 0x44, 0x09, 0x5C,
    0x8D, 0xCC, 0x87, 0x88, 0x2D, 0x8A, 0xEB, 0x2C,
    0xB5, 0xE0, 0x32, 0xAD, 0xD3, 0x61, 0xAE, 0x15,
    0x60, 0xF1, 0x48, 0x0E, 0x7E, 0x94, 0x51, 0x0C,
    0xEC, 0xDB, 0xD2, 0x64, 0xDC, 0xFC, 0xC2, 0x56,
    0x24, 0xED, 0x2B, 0xD9, 0x1C, 0x68, 0x90, 0x79
};

XoringStreamAdapter::XoringStreamAdapter(IRandomAccessStream* ras)
    : BitStreamAdapter(ras), _key(0x7f) { }

unsigned XoringStreamAdapter::readSome(void *dest, unsigned byteCount) {
    unsigned bytesRead = BitStreamAdapter::readSome(dest, byteCount);
    auto bytes = static_cast<unsigned char*>(dest);
    for (unsigned i = 0; i < byteCount; ++i) {
        unsigned char byte = bytes[i];
        bytes[i] ^= _key;
        _key = xor_pad[byte];
    }
    return bytesRead;
}

void XoringStreamAdapter::seek(unsigned pos) {
    BitStreamAdapter::seek(pos);
    _key = 0x7f;
}

FileStream::FileStream(std::filesystem::path path)
    : _file(openForReading(path)) { }

unsigned FileStream::readSome(void *dest, unsigned byteCount) {
    _file.read(reinterpret_cast<char*>(dest), byteCount);
    auto bytesRead = _file.gcount();
    _pos += bytesRead;
    return bytesRead;
}

void FileStream::seek(unsigned pos) {
    if (_pos != pos) {
        _file.seekg(pos);
        _pos = pos;
    }
}

unsigned FileStream::tell() {
    return _pos;
}

uint8_t read8(IRandomAccessStream* stream) {
    uint8_t res;
    stream->readSome(&res, sizeof res);
    return res;
}

uint16_t read16(IRandomAccessStream* stream) {
    uint16_t res;
    stream->readSome(&res, sizeof res);
    return res;
}

uint32_t read32(IRandomAccessStream* stream) {
    uint32_t res;
    stream->readSome(&res, sizeof res);
    return res;
}

bool readLine(IRandomAccessStream* stream, std::string& line, char sep) {
    line.resize(0);
    char ch;
    for (;;) {
        if (!stream->readSome(&ch, 1))
            return !line.empty();
        if (ch == sep)
            return true;
        line += ch;
    }
}

uint32_t peek32(IRandomAccessStream *stream) {
    auto value = read32(stream);
    stream->seek(stream->tell() - 4);
    return value;
}

}
