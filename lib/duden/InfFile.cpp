#include "InfFile.h"

#include "Duden.h"
#include "LdFile.h"
#include "boost/algorithm/string.hpp"

namespace duden {

namespace {

std::string fixFileNameCase(const std::string& name, const CaseInsensitiveSet& files) {
    if (name.empty())
        return name;
    auto it = files.find(std::filesystem::u8path(name));
    if (it != end(files)) {
        return it->filename().u8string();
    }
    return name;
}

} // namespace

std::vector<InfFile> parseInfFile(common::IRandomAccessStream* stream, IFileSystem* filesystem) {
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
                auto value = line.substr(index + 1);
                boost::algorithm::trim_if(value, boost::algorithm::is_any_of("\r "));
                lds.push_back(fixFileNameCase(value, fsFileSet));
            } break;
            case 'F': {
                auto index = line.find(';');
                if (index == std::string::npos)
                    throw std::runtime_error("INF file syntax error");
                auto value = line.substr(index + 1);
                boost::algorithm::trim_if(value, boost::algorithm::is_any_of("\r "));
                files.insert(std::filesystem::u8path(fixFileNameCase(value, fsFileSet)));
            } break;
        }
    }

    std::vector<InfFile> infs;

    auto findExt = [&](auto ext) {
        return std::find_if(begin(files), end(files), [&](auto file) {
            auto str = file.u8string();
            boost::algorithm::to_lower(str);
            return boost::algorithm::ends_with(str, ext);
        });
    };

    for (auto& name : lds) {
        auto ld = parseLdFile(filesystem->open(name).get());
        InfFile inf;
        inf.version = version;
        inf.supported = true;
        inf.ld = name;
        inf.primary.hic = fixFileNameCase(ld.baseFileName + ".hic", files);
        inf.primary.idx = fixFileNameCase(std::filesystem::u8path(name).stem().u8string() + ".idx", files);
        inf.primary.bof = fixFileNameCase(std::filesystem::u8path(name).stem().u8string() + ".bof", files);
        files.erase(inf.primary.bof);
        files.erase(inf.primary.idx);
        infs.push_back(inf);
    }

    ResourceArchive fsiResource;
    auto fsi = findExt(".fsi");
    if (fsi != end(files)) {
        fsiResource.fsi = fsi->string();
        auto bof = files.find(std::filesystem::u8path(fsi->stem().u8string() + ".bof"));
        auto idx = files.find(std::filesystem::u8path(fsi->stem().u8string() + ".idx"));
        if (bof == end(files) || idx == end(files)) {
            fsiResource = {};
            fsi = end(files);
        } else {
            fsiResource.bof = fixFileNameCase(bof->string(), files);
            fsiResource.idx = fixFileNameCase(idx->string(), files);
            files.erase(std::filesystem::u8path(fsiResource.bof));
        }
    }

    auto filesCopy = files;
    for (auto& inf : infs) {
        if (!fsiResource.fsi.empty()) {
            inf.resources.push_back(fsiResource);
        }

        for (;;) {
            auto fsd = findExt(".fsd");
            if (fsd != end(files)) {
                ResourceArchive resource;
                resource.fsd = fsd->string();
                files.erase(fsd);
                auto base = std::filesystem::u8path(resource.fsd).stem().u8string();
                resource.fsi = fixFileNameCase(base + ".fsi", files);
                if (files.find(resource.fsi) == end(files))
                    throw std::runtime_error("FSD blob doesn't have a corresponding FSI file");
                inf.resources.push_back(resource);
                continue;
            }

            auto bof = findExt(".bof");
            if (bof == end(files))
                break;
            ResourceArchive resource;
            resource.bof = bof->string();
            files.erase(bof);
            auto base = std::filesystem::u8path(resource.bof).stem().u8string();
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
