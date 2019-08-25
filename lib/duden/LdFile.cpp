#include "LdFile.h"
#include "Duden.h"
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

namespace duden {

LdFile parseLdFile(dictlsd::IRandomAccessStream* stream) {
    LdFile ld;

    ld.references.push_back({"WEB", "Web", "W"});

    std::string line;
    while (readLine(stream, line)) {
        boost::algorithm::trim(line);
        if (line.empty())
            continue;
        line = win1252toUtf8(line);
        if (line[0] == 'G' || line[0] == 'g') {
            boost::smatch m;
            if (!boost::regex_match(line, m, boost::regex("^.(.*?)\\|(.*?)\\|(.*?)$")))
                throw std::runtime_error("LD parsing error");
            ld.references.push_back({m[1], m[2], m[3]});
        } else if (line[0] == 'B') {
            ld.name = line.substr(1);
        } else if (line[0] == 'S') {
            ld.sourceLanguage = line.substr(1);
        } else if (line[0] == 'K') {
            ld.baseFileName = line.substr(1);
        } else if (line[0] == 'D') {
            boost::smatch m;
            if (!boost::regex_match(line, m, boost::regex("^D(.+?) (\\d+) (\\d+).*$")))
                throw std::runtime_error("LD parsing error");
            ld.ranges.push_back({m[1],
                                 static_cast<uint32_t>(std::stoul(m[2])),
                                 static_cast<uint32_t>(std::stoul(m[3]))});
        }
    }
    return ld;
}

int dudenLangToCode(const std::string& lang) {
    if (lang == "deu")
        return 1031;
    if (lang == "enu")
        return 1033;
    if (lang == "fra")
        return 1036;
    if (lang == "esn")
        return 1034;
    if (lang == "ita")
        return 1040;
    if (lang == "rus")
        return 1049;
    return 0;
}

void updateLanguageCodes(std::vector<LdFile*> lds) {
    auto first = lds.at(0);
    auto second = lds.size() > 1 ? lds.at(1) : nullptr;
    auto firstSource = first->sourceLanguage;
    auto secondSource = second ? second->sourceLanguage : "deu";

    first->sourceLanguageCode = dudenLangToCode(firstSource);
    first->targetLanguageCode = dudenLangToCode(secondSource);

    if (second) {
        second->sourceLanguageCode = dudenLangToCode(secondSource);
        second->targetLanguageCode = dudenLangToCode(firstSource);
    }
}

} // namespace duden
