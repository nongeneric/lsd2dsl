#include "InfFile.h"

#include "Duden.h"
#include "boost/algorithm/string.hpp"

namespace duden {

namespace {

std::string fixFileNameCase(const std::string& name, const CaseInsensitiveSet& files) {
    if (name.empty())
        return name;
    auto it = files.find(fs::path(name));
    if (it != end(files)) {
        return it->filename().string();
    }
    return name;
}

} // namespace

std::vector<InfFile> parseInfFile(dictlsd::IRandomAccessStream* stream, IFileSystem* filesystem) {
    std::string line;
    uint32_t version = 0;
    CaseInsensitiveSet files;
    std::vector<std::string> lds;

    const auto& fsFileSet = filesystem->files();

    while (readLine(stream, line)) {
        if (line.empty())
            continue;
        line = win1252toUtf8(line);
        switch (line[0]) {
            case 'V': version = std::stoi(line.substr(2), nullptr, 16); break;
            case 'L': {
                auto index = line.find(' ', 2);
                if (index == std::string::npos)
                    throw std::runtime_error("INF file syntax error");
                boost::algorithm::trim_right_if(line, boost::algorithm::is_any_of("\r"));
                lds.push_back(fixFileNameCase(line.substr(index + 1), fsFileSet));
            } break;
            case 'F': {
                auto index = line.find(';');
                if (index == std::string::npos)
                    throw std::runtime_error("INF file syntax error");
                boost::algorithm::trim_right_if(line, boost::algorithm::is_any_of("\r"));
                files.insert(fs::path(fixFileNameCase(line.substr(index + 1), fsFileSet)));
            } break;
        }
    }

    std::vector<InfFile> infs;    

    auto findExt = [&](auto ext) {
        return std::find_if(begin(files), end(files), [&](auto file) {
            auto str = file.string();
            boost::algorithm::to_lower(str);
            return boost::algorithm::ends_with(str, ext);
        });
    };

    for (auto& ld : lds) {
        InfFile inf;
        inf.version = version;
        inf.supported = true;
        inf.ld = ld;
        inf.primary.hic = fixFileNameCase(fs::path(ld).stem().string() + ".hic", files);
        inf.primary.idx = fixFileNameCase(fs::path(ld).stem().string() + ".idx", files);
        inf.primary.bof = fixFileNameCase(fs::path(ld).stem().string() + ".bof", files);
        files.erase(inf.primary.bof);
        files.erase(inf.primary.idx);
        infs.push_back(inf);
    }

    ResourceArchive fsiResource;
    auto fsi = findExt(".fsi");
    if (fsi != end(files)) {
        fsiResource.fsi = fsi->string();
        auto bof = files.find(fsi->stem().string() + ".bof");
        auto idx = files.find(fsi->stem().string() + ".idx");
        if (bof == end(files) || idx == end(files)) {
            fsiResource = {};
            files.erase(fsi);
            fsi = end(files);
        } else {
            fsiResource.bof = fixFileNameCase(bof->string(), files);
            fsiResource.idx = fixFileNameCase(idx->string(), files);
            files.erase(fs::path(fsiResource.bof));
        }
    }

    auto filesCopy = files;
    for (auto& inf : infs) {
        if (!fsiResource.fsi.empty()) {
            inf.resources.push_back(fsiResource);
        }

        for (;;) {
            auto bof = findExt(".bof");
            if (bof == end(files))
                break;
            ResourceArchive resource;
            resource.bof = bof->string();
            files.erase(bof);
            auto base = fs::path(resource.bof).stem().string();
            resource.fsi = fixFileNameCase(base + ".fsi", files);
            resource.idx = fixFileNameCase(base + ".idx", files);
            if (files.find(resource.fsi) == end(files)) {
                resource.fsi = "";
            }
            inf.resources.push_back(resource);
        }

        files = filesCopy;
    }

    return infs;
}

} // namespace duden
