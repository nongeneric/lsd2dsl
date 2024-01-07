#pragma once

#include "Log.h"
#include <functional>
#include <string>
#include <vector>
#include <string_view>
#include <fstream>
#include <filesystem>

namespace dsl {

class Writer {
    std::unique_ptr<std::ofstream> _dsl;
    std::filesystem::path _dslPath;
    void write(const std::u16string &line);

public:
    Writer(std::filesystem::path outputPath, std::string name);
    std::filesystem::path dslFileName() const;
    std::filesystem::path dslFilePath() const;
    void setName(std::u16string name);
    void setAnnotation(std::u16string annotation);
    void setLanguage(int source, int target);
    void setIcon(std::vector<uint8_t> icon);
    void writeNewLine();
    void writeHeading(std::u16string heading);
    void writeArticle(std::u16string article);
};

} // namespace dsl
