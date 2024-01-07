#pragma once

#include "common/BitStream.h"
#include "common/CommonTools.h"
#include <boost/lexical_cast.hpp>
#include <stdint.h>
#include <string>
#include <ostream>
#include <vector>
#include <fstream>
#include <filesystem>

namespace lingvo {

class LenTable;

unsigned UpperPrimeNumber(unsigned count);
unsigned BitLength(unsigned num);
std::u16string readUnicodeString(common::IBitStream* bstr, int len, bool bigEndian);
std::vector<char32_t> readSymbols(common::IBitStream* bstr);
bool readReference(common::IBitStream& bstr, unsigned& reference, unsigned huffmanNumber);
uint16_t reverse16(uint16_t n);
uint32_t reverse32(uint32_t n);
int majorVersion(unsigned dictVersion);
int minorVersion(unsigned dictVersion);
int revisionVersion(unsigned dictVersion);

}
