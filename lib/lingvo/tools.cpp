#include "tools.h"
#include "common/BitStream.h"
#include "LenTable.h"

#include <fmt/format.h>
#include <map>
#include <assert.h>

namespace lingvo {

int majorVersion(unsigned dictVersion) {
    return dictVersion >> 16;
}

int minorVersion(unsigned dictVersion) {
    return (dictVersion >> 12) & 0x0F;
}

int revisionVersion(unsigned dictVersion) {
    return dictVersion & 0xFFF;
}

unsigned UpperPrimeNumber(unsigned count) {
    if (count < 0x35)
        return 0x35;
    assert(false);
    return 0;
}

unsigned BitLength(unsigned num) {
    unsigned res = 1;
    while ((num >>= 1) != 0)
        res++;
    return res;
}

std::u16string readUnicodeString(common::IBitStream* bstr, int len, bool bigEndian) {
    std::u16string str;
    for (int i = 0; i < len; ++i) {
        uint16_t ch;
        bstr->readSome(&ch, 2);
        str += bigEndian ? reverse16(ch) : ch;
    }
    return str;
}

uint16_t reverse16(uint16_t n) {
    char* arr = (char*)&n;
    std::swap(arr[0], arr[1]);
    return n;
}

uint32_t reverse32(uint32_t n) {
    char* arr = (char*)&n;
    std::swap(arr[0], arr[3]);
    std::swap(arr[1], arr[2]);
    return n;
}

std::vector<char32_t> readSymbols(common::IBitStream* bstr) {
    int len = bstr->read(32);
    int bitsPerSymbol = bstr->read(8);
    std::vector<char32_t> res;
    for (int i = 0; i < len; i++) {
        char32_t symbol = bstr->read(bitsPerSymbol);
        res.push_back(symbol);
    }
    return res;
}

bool readReference(common::IBitStream& bstr, unsigned& reference, unsigned huffmanNumber) {
    int code = bstr.read(2);
    if (code == 3) {
        reference = bstr.read(32);
        return true;
    }
    int bitlen = BitLength(huffmanNumber);
    assert(bitlen >= 2);
    reference = (code << (bitlen - 2)) | bstr.read(bitlen - 2);
    return true;
}

}
