#include "IFileSystem.h"
#include "lib/common/bformat.h"
#include "lib/common/filesystem.h"
#include <QString>
#include <algorithm>

namespace duden {

bool CaseInsensitiveLess::operator()(const fs::path& left,
                                     const fs::path& right) const {
    auto qleft = QString::fromStdString(left.string());
    auto qright = QString::fromStdString(right.string());
    return qleft.compare(qright, Qt::CaseInsensitive) < 0;
}

} // namespace duden
