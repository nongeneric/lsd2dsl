#include "IFileSystem.h"
#include "lib/common/bformat.h"
#include <boost/algorithm/string.hpp>
#include <algorithm>

namespace duden {

fs::path findCaseInsensitive(IFileSystem& fileSystem, fs::path fileName) {
    auto lowerNameToFind = boost::to_lower_copy(fileName.string());
    for (auto& path : fileSystem.files()) {
        auto name = boost::to_lower_copy(path.filename().string());
        if (name == lowerNameToFind) {
            return path;
        }
    }
    throw std::runtime_error("file not found: " + fileName.string());
}

fs::path findExtension(IFileSystem& fileSystem, fs::path name) {
    std::vector<fs::path> results;
    auto files = fileSystem.files();
    std::copy_if(
        begin(files), end(files), std::back_inserter(results), [&](auto& p) {
            auto lower = boost::to_lower_copy(p.extension().string());
            return lower == name;
        });
    if (results.size() > 1)
        throw std::runtime_error(bformat("found multiple files with extension %s", name));
    if (results.empty())
        throw std::runtime_error(bformat("file with extension %s not found", name));
    return results.front();
}

} // namespace duden
