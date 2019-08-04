#include "InfFile.h"

#include "Duden.h"
#include "boost/algorithm/string.hpp"

namespace duden {

InfFile parseInfFile(dictlsd::IRandomAccessStream* stream) {
    InfFile inf;
    std::string line;
    std::vector<std::string> files;

    while (readLine(stream, line)) {
        if (line.empty())
            continue;
        line = win1252toUtf8(line);
        switch (line[0]) {
        case 'V':
            inf.version = std::stoi(line.substr(2), nullptr, 16);
            break;
        case 'Q':
            inf.name = line.substr(2);
            boost::algorithm::trim_if(inf.name, boost::algorithm::is_any_of("\r\""));
            break;
        case 'F':
            auto index = line.find(';');
            if (index == std::string::npos)
                throw std::runtime_error("INF file syntax error");
            boost::algorithm::trim_right_if(line, boost::algorithm::is_any_of("\r"));
            files.push_back(line.substr(index + 1));
            break;
        }
    }

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

    inf.supported = true;

    return inf;
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
