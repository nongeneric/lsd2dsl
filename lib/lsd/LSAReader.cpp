#include "LSAReader.h"

#include "OggReader.h"
#include "lib/common/WavWriter.h"
#include "lib/common/filesystem.h"
#include "BitStream.h"
#include "tools.h"
#include "UnicodePathFile.h"
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
        unsigned sampleOffset = 0;
        if (i > 0) {
            _bstr->readSome(&sampleOffset, 4);
            uint8_t marker;
            _bstr->readSome(&marker, 1);
            if (marker == 0) // group
                continue;
            if (marker != 0xFF)
                throw std::runtime_error("bad LSA file");
        }
        unsigned size;
        _bstr->readSome(&size, 4);
        _totalSamples += size;
        _entries.push_back({name, sampleOffset, size});
    }
    _oggOffset = _bstr->tell();
}

void LSAReader::dump(std::string path,
                     Log& log)
{
    _bstr->seek(_oggOffset);
    OggReader oggReader(_bstr);
    std::vector<short> samples;
    std::vector<char> wav;
    uint64_t curSample = 0;

    for (LSAEntry& entry : _entries) {
        if (curSample != entry.sampleOffset) { // int32 overflow, ignore
        }

        std::string name = toUtf8(entry.name);
        boost::algorithm::trim(name);

        oggReader.readSamples(entry.sampleSize, samples);
        createWav(samples, wav);

        if (samples.size() != entry.sampleSize)
            throw std::runtime_error("error reading LSA");

        UnicodePathFile file(path + "/" + name, true);
        file.write(wav.data(), wav.size());

        log.advance();
        curSample += samples.size();
    }
}

unsigned LSAReader::entriesCount() const {
    return _entriesCount;
}

void decodeLSA(std::string lsaPath, std::string outputPath, Log& log) {
    fs::path lsaOutputDir = outputPath / fs::path(lsaPath).filename().replace_extension("extracted");
    fs::create_directories(lsaOutputDir);
    FileStream bstr(lsaPath);
    LSAReader reader(&bstr);
    reader.collectHeadings();
    log.resetProgress(fs::path(lsaPath).filename().string(), reader.entriesCount());
    reader.dump(lsaOutputDir.string(), log);
}

}
