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

} // namespace duden
