#include "IFileSystem.h"
#include "common/bformat.h"
#include <QString>
#include <algorithm>

namespace duden {

bool CaseInsensitiveLess::operator()(const std::filesystem::path& left,
                                     const std::filesystem::path& right) const {
    auto qleft = QString::fromStdString(left.u8string());
    auto qright = QString::fromStdString(right.u8string());
    return qleft.compare(qright, Qt::CaseInsensitive) < 0;
}

} // namespace duden
