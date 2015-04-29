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

	mpg123_close(f);
	mpg123_delete(f);
    
    return true;
}
