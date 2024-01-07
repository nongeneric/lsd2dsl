#pragma once

#include <filesystem>
#include <fstream>

std::ofstream openForWriting(std::filesystem::path path);
std::ifstream openForReading(std::filesystem::path path);
std::u16string langFromCode(int code);
void printLanguages();
std::string toUtf8(std::u16string u16str);
std::u16string toUtf16(std::string u8str);
