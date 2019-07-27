#pragma once

#include "Log.h"
#include "lib/lsd/lsd.h"
#include "lib/lsd/UnicodePathFile.h"
#include <boost/filesystem.hpp>
#include <functional>
#include <string>
#include <vector>
#include <string_view>

void writeDSL(const dictlsd::LSDDictionary* reader,
              std::string lsdName,
              std::string outputPath,
              bool dumb,
              Log& log);

namespace dsl {

class Writer {
    std::unique_ptr<UnicodePathFile> _dsl;
    boost::filesystem::path _dslPath;
    void write(const std::u16string &line);

public:
    Writer(std::string outputPath, std::string name);
    std::string dslFileName() const;
    void setName(std::u16string name);
    void setAnnotation(std::u16string annotation);
    void setLanguage(int source, int target);
    void setIcon(std::vector<uint8_t> icon);
    void writeHeading(std::u16string heading);
    void writeArticle(std::u16string article);
};

} // namespace dsl
