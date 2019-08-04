#include "IFileSystem.h"
#include "lib/common/bformat.h"
#include <QString>
#include <algorithm>

namespace duden {

fs::path findExtension(IFileSystem& fileSystem, fs::path name) {
    std::vector<fs::path> results;
    auto files = fileSystem.files();
    auto qstrName = QString::fromStdString(name.string());
    std::copy_if(
        begin(files), end(files), std::back_inserter(results), [&](auto& p) {
            return qstrName.compare(QString::fromStdString(p.extension().string()), Qt::CaseInsensitive) == 0;
        });
    if (results.size() > 1)
        throw std::runtime_error(bformat("found multiple files with extension %s", name));
    if (results.empty())
        throw std::runtime_error(bformat("file with extension %s not found", name));
    return results.front();
}

bool CaseInsensitiveLess::operator()(const boost::filesystem::path& left,
                                     const boost::filesystem::path& right) const {
    auto qleft = QString::fromStdString(left.string());
    auto qright = QString::fromStdString(right.string());
    return qleft.compare(qright, Qt::CaseInsensitive) < 0;
}

} // namespace duden
