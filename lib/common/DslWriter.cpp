#include "DslWriter.h"

#include "ZipWriter.h"
#include "common/CommonTools.h"

#include <fmt/format.h>

namespace dsl {

    constexpr char _utf16bom[] { (char)0xff, (char)0xfe };

    void Writer::write(const std::u16string& line) {
        _dsl->write((char*)line.c_str(), 2 * line.length());
    }

    Writer::Writer(std::filesystem::path outputPath, std::string name) {
        _dslPath = outputPath / (name + ".dsl");
        _dsl = std::make_unique<std::ofstream>(_dslPath, std::ios::binary);
        if (!_dsl->is_open())
            throw std::runtime_error(
                fmt::format("Can't open file for writing {}", _dslPath.u8string()));
        _dsl->write(_utf16bom, sizeof(_utf16bom));
    }

    std::filesystem::path Writer::dslFileName() const {
        return _dslPath.filename();
    }

    std::filesystem::path Writer::dslFilePath() const {
        return _dslPath;
    }

    void Writer::setName(std::u16string name) {
        write(u"#NAME\t\"" + name + u"\"\r\n");
    }

    void Writer::setAnnotation(std::u16string annotation) {
        auto path = _dslPath;
        path.replace_extension("ann");
        auto anno = openForWriting(path);
        anno.write(_utf16bom, sizeof(_utf16bom));
        anno.write((char*)annotation.c_str(), 2 * annotation.length());
    }

    void Writer::setLanguage(int source, int target) {
        write(u"#INDEX_LANGUAGE\t\"" + langFromCode(source) + u"\"\n");
        write(u"#CONTENTS_LANGUAGE\t\"" + langFromCode(target) + u"\"\n");
    }

    void Writer::setIcon(std::vector<uint8_t> icon) {
        auto path = _dslPath;
        path.replace_extension("bmp");

        write(u"#ICON_FILE\t\"" + toUtf16(path.filename().u8string()) + u"\"\n");

        auto file = openForWriting(path);
        file.write(reinterpret_cast<const char*>(icon.data()), icon.size());
    }

    void Writer::writeNewLine() {
        write(u"\n");
    }

    void Writer::writeHeading(std::u16string heading) {
        write(heading.c_str());
        write(u"\n");
    }

    void Writer::writeArticle(std::u16string article) {
        std::u16string tab = u"\t";
        write(tab);
        for (auto ch : article) {
            _dsl->write((char*)&ch, sizeof(ch));
            if (ch == u'\n') {
                write(tab);
            }
        }

        write(u"\n");
    }

} // namespace dsl
