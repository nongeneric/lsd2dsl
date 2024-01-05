#include "WavWriter.h"

#include <sndfile.h>
#include <string.h>
#include <stdexcept>
#include <assert.h>

namespace dictlsd {

struct vio_vec {
    std::vector<char>* vec;
    unsigned pos;
};

sf_count_t vio_vec_get_filelen(void *user_data) {
    vio_vec* vec = static_cast<vio_vec*>(user_data);
    return vec->vec->size();
}

sf_count_t vio_vec_seek(sf_count_t offset, int whence, void *user_data) {
    vio_vec* vec = static_cast<vio_vec*>(user_data);
    unsigned pos;
    if (whence == SEEK_SET) {
        pos = offset;
    } else if (whence == SEEK_CUR) {
        pos = vec->pos + offset;
    } else {
        pos = vec->vec->size() - offset;
    }
    return vec->pos = std::min((size_t)pos, vec->vec->size());
}

sf_count_t vio_vec_read(void *ptr, sf_count_t count, void *user_data) {
    vio_vec* vec = static_cast<vio_vec*>(user_data);
    count = std::min((size_t)count, vec->vec->size() - vec->pos);
    memcpy(ptr, vec->vec->data(), count);
    return count;
}

sf_count_t vio_vec_write(const void *ptr, sf_count_t count, void *user_data) {
    vio_vec* vec = static_cast<vio_vec*>(user_data);
    if (vec->pos + (unsigned)count > vec->vec->size()) {
        vec->vec->resize(vec->pos + count);
    }
    memcpy(&(*vec->vec)[vec->pos], ptr, count);
    vec->pos += count;
    return count;
}

sf_count_t vio_vec_tell(void *user_data) {
    vio_vec* vec = static_cast<vio_vec*>(user_data);
    return vec->pos;
}

SF_VIRTUAL_IO vio_vec_callbacks {
    vio_vec_get_filelen,
    vio_vec_seek,
    vio_vec_read,
    vio_vec_write,
    vio_vec_tell
};

void createWav(std::vector<int16_t> const& samples, std::vector<char> &wav, int rate, int channels) {
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof sfinfo);
    sfinfo.samplerate = rate;
    sfinfo.channels = channels;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    wav.clear();
    vio_vec vec = { &wav, 0 };
    SNDFILE* outfile = sf_open_virtual(&vio_vec_callbacks, SFM_WRITE, &sfinfo, &vec);
    if (!outfile) {
        throw std::runtime_error("can't create wav");
    }
    unsigned written = sf_writef_short(outfile, samples.data(), samples.size());
    if (written != samples.size())
        throw std::runtime_error("can't write wav file");
    sf_close(outfile);
}

}
