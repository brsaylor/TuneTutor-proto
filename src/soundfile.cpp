#include <iostream>
#include <string>
#include <vector>

extern "C" {
#include <mpg123.h>
}

#include "soundfile.h"

SoundFile::SoundFile() {
    sampleRate = 0;
    channels = 0;
}

bool SoundFile::load(std::string path) {
    return loadMp3(path);
}

int SoundFile::getSampleRate() {
    return sampleRate;
}

int SoundFile::getChannels() {
    return channels;
}

std::vector<float> SoundFile::getSamples() {
    return samples;
}

SoundFileMetadata SoundFile::getMetadata() {
    return metadata;
}

bool SoundFile::loadMp3(std::string path) {
	int err = MPG123_OK;
    mpg123_init();
	mpg123_handle *f = mpg123_new(NULL, &err);
    mpg123_param(f, MPG123_ADD_FLAGS, MPG123_FORCE_FLOAT, 0.);
	if ((err = mpg123_open(f, path.c_str())) != MPG123_OK) {
        std::cout << "loadMp3(): mpg123_open() returned " << err << "\n";
		return false;
	}

	mpg123_enc_enum encoding;
    long rate;
	mpg123_getformat(f, &rate, &channels, (int*) &encoding);
    sampleRate = rate;

	size_t done=0;
    samples.resize(mpg123_length(f) * channels);
    mpg123_read(f, (unsigned char *) &(samples[0]),
            samples.size() * sizeof(float), &done);

    // Get the metadata
    mpg123_id3v1 *id3v1;
    mpg123_id3v2 *id3v2;
    mpg123_id3(f, &id3v1, &id3v2);
    if (id3v2 != NULL) {
        std::cout << "id3v2 data found" << std::endl;
        metadata.title = id3v2->title->p;
        metadata.artist = id3v2->artist->p;
        metadata.album = id3v2->album->p;
    } else if (id3v1 != NULL) {
        std::cout << "id3v1 data found" << std::endl;
        metadata.title = std::string(id3v1->title, 30);
        metadata.artist = std::string(id3v1->artist, 30);
        metadata.album = std::string(id3v1->album, 30);
    } else {
        std::cout << "No id3 data found" << std::endl;
    }

	mpg123_close(f);
	mpg123_delete(f);
    
    return true;
}
