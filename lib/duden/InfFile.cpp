#include "InfFile.h"

#include "Duden.h"
#include "boost/algorithm/string.hpp"

namespace duden {

std::vector<InfFile> parseInfFile(dictlsd::IRandomAccessStream* stream) {
    std::string line;
    uint32_t version = 0;
    std::vector<std::vector<std::string>> allFiles;
    std::vector<std::string> currentFiles;
    std::vector<std::string> names;

    while (readLine(stream, line)) {
        if (line.empty())
            continue;
        line = win1252toUtf8(line);
        switch (line[0]) {
        case 'V':
            version = std::stoi(line.substr(2), nullptr, 16);
            break;
        case 'T': {
            if (!currentFiles.empty()) {
                allFiles.push_back(currentFiles);
                currentFiles.clear();
            }
            auto name = line.substr(2);
            boost::algorithm::trim_if(name, boost::algorithm::is_any_of("\r\""));
            names.push_back(name);
        } break;
        case 'F':
            auto index = line.find(';');
            if (index == std::string::npos)
                throw std::runtime_error("INF file syntax error");
            boost::algorithm::trim_right_if(line, boost::algorithm::is_any_of("\r"));
            currentFiles.push_back(line.substr(index + 1));
            break;
        }
    }

    allFiles.push_back(currentFiles);

    std::vector<InfFile> infs;

    for (auto i = 0u; i < allFiles.size(); ++i) {
        auto& files = allFiles[i];

        InfFile inf;
        inf.version = version;
        inf.supported = true;
        inf.name = names[i];

        auto findExt = [&](auto ext) {
            return std::find_if(begin(files), end(files), [&](auto file) {
                boost::algorithm::to_lower(file);
                return boost::algorithm::ends_with(file, ext);
            });
        };

        auto hic = findExt(".hic");

        if (hic == end(files))
            throw std::runtime_error("dictionary doesn't contain a HIC file");

        inf.primary.hic = *hic;

        auto baseName = [] (auto name) {
            boost::algorithm::to_lower(name);
            return name.substr(0, name.size() - 4);
        };

        auto primaryBaseName = baseName(*hic);

        auto find = [&](auto name) {
            return std::find_if(begin(files), end(files), [&](auto file) {
                boost::algorithm::to_lower(file);
                return file == name;
            });
        };

        auto primaryBof = find(primaryBaseName + ".bof");
        auto primaryIdx = find(primaryBaseName + ".idx");

        if (primaryBof == end(files) || primaryIdx == end(files))
            throw std::runtime_error("dictionary doesn't contain an IDX or BOF file");

        inf.primary.bof = *primaryBof;
        inf.primary.idx = *primaryIdx;

        files.erase(primaryBof);

        for (;;) {
            auto bof = findExt(".bof");
            if (bof == end(files))
                break;
            ResourceArchive resource;
            resource.bof = *bof;
            files.erase(bof);
            auto base = baseName(resource.bof);
            auto fsi = find(base + ".fsi");
            auto idx = find(base + ".idx");
            if (idx == end(files))
                throw std::runtime_error("a resource archive doesn't have a corresponding IDX or FSI file");
            if (fsi != end(files)) {
                resource.fsi = *fsi;
            }
            resource.idx = *idx;
            inf.resources.push_back(resource);
        }

        infs.push_back(inf);
    }

    return infs;
}

void fixFileNameCase(std::string& name, IFileSystem* filesystem) {
    if (name.empty())
        return;
    auto& files = filesystem->files();
    auto it = files.find(fs::path(name));
    assert(it != end(files));
    name = it->filename().string();
}

void fixFileNameCase(InfFile& inf, IFileSystem* filesystem) {
    fixFileNameCase(inf.primary.bof, filesystem);
    fixFileNameCase(inf.primary.hic, filesystem);
    fixFileNameCase(inf.primary.idx, filesystem);
    for (auto& resource : inf.resources) {
        fixFileNameCase(resource.bof, filesystem);
        fixFileNameCase(resource.fsi, filesystem);
        fixFileNameCase(resource.idx, filesystem);
    }
}

} // namespace duden
