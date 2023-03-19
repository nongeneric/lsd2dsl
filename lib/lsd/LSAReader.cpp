#include "LSAReader.h"

#include "OggReader.h"
#include "lib/common/WavWriter.h"
#include "BitStream.h"
#include "tools.h"
#include <stdexcept>
#include <assert.h>
#include <boost/algorithm/string.hpp>

namespace dictlsd {

std::u16string readLSAString(IRandomAccessStream* bstr) {
    std::u16string res;
    unsigned char chr, nextchr;
    for (;;) {
        bstr->readSome(&chr, 1);
        if (chr == 0xFF)
            break;
        bstr->readSome(&nextchr, 1);
        if (nextchr == 0xFF)
            break;
        res += chr | (nextchr << 8);
    }
    return res;
}

LSAReader::LSAReader(IRandomAccessStream *bstr)
    : _bstr(bstr)
{
    assert(bstr);
    std::u16string magic = readLSAString(_bstr);
    if (magic != u"L9SA")
        throw std::runtime_error("not an LSA archive");
    _bstr->readSome(&_entriesCount, 4);
}

void LSAReader::collectHeadings() {
    _totalSamples = 0;
    for (size_t i = 0; i < _entriesCount; ++i) {
        std::u16string name = readLSAString(_bstr);
        uint32_t sampleOffset = 0;
        if (i > 0) {
            _bstr->readSome(&sampleOffset, 4);
            uint8_t marker;
            _bstr->readSome(&marker, 1);
            if (marker == 0) // group
                continue;
            if (marker != 0xFF)
                throw std::runtime_error("bad LSA file");
        }
        uint32_t size;
        _bstr->readSome(&size, 4);
        _totalSamples += size;
        _entries.push_back({name, sampleOffset, size});
    }
    _oggOffset = _bstr->tell();
}

void LSAReader::dump(std::filesystem::path path, Log& log) {
    _bstr->seek(_oggOffset);
    OggReader oggReader(_bstr);
    std::vector<short> samples;
    std::vector<char> wav;

    for (size_t i = 0; i < _entries.size(); ++i) {
        auto& entry = _entries[i];
        auto fileSampleSize = entry.sampleSize;
        if (i != _entries.size() - 1) {
            fileSampleSize = _entries[i + 1].sampleOffset - _entries[i].sampleOffset;
        }

        std::string name = toUtf8(entry.name);
        boost::algorithm::trim(name);

        oggReader.readSamples(fileSampleSize, samples);
        samples.resize(entry.sampleSize);
        auto info = oggReader.info();
        createWav(samples, wav, info.rate, info.channels);

        if (samples.size() != entry.sampleSize)
            throw std::runtime_error("error reading LSA");

        auto file = openForWriting(path / std::filesystem::u8path(name));
        file.write(wav.data(), wav.size());

        log.advance();
    }
}

unsigned LSAReader::entriesCount() const {
    return _entriesCount;
}

void decodeLSA(std::filesystem::path lsaPath, std::filesystem::path outputPath, Log& log) {
    auto lsaOutputDir = outputPath / lsaPath.filename().replace_extension("extracted");
    std::filesystem::create_directories(lsaOutputDir);
    FileStream bstr(lsaPath);
    LSAReader reader(&bstr);
    reader.collectHeadings();
    log.resetProgress(lsaPath.filename().u8string(), reader.entriesCount());
    reader.dump(lsaOutputDir, log);
}

}
