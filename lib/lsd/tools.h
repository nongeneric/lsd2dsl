#pragma once

#include "BitStream.h"
#include <boost/lexical_cast.hpp>
#include <stdint.h>
#include <string>
#include <ostream>
#include <vector>
#include <fstream>
#include <filesystem>

namespace dictlsd {

class LenTable;

unsigned UpperPrimeNumber(unsigned count);
unsigned BitLength(unsigned num);
std::u16string readUnicodeString(IBitStream* bstr, int len, bool bigEndian);
std::vector<char32_t> readSymbols(IBitStream* bstr);
bool readReference(IBitStream& bstr, unsigned& reference, unsigned huffmanNumber);
uint16_t reverse16(uint16_t n);
uint32_t reverse32(uint32_t n);
std::string toUtf8(std::u16string u16str);
std::u16string toUtf16(std::string u8str);
std::u16string langFromCode(int code);
void printLanguages();
int majorVersion(unsigned dictVersion);
int minorVersion(unsigned dictVersion);
int revisionVersion(unsigned dictVersion);
std::ofstream openForWriting(std::filesystem::path path);
std::ifstream openForReading(std::filesystem::path path);

}
