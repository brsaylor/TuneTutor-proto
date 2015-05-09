#pragma once

#include <string>
#include <vector>

/**
 * Simple container for metadata fields extracted from a sound file.
 */
struct SoundFileMetadata {
    std::string title;
    std::string artist;
    std::string album;

    SoundFileMetadata() {
        title = "";
        artist = "";
        album = "";
    }
};

/**
 * Loads a sound file and provides access to its metadata and samples.
 */
class SoundFile {

    public:
        SoundFile();

        /**
         * Load the given file's metadata and sample data into memory.
         * @param path the full path to the file
         */
        bool load(std::string path);

        int getSampleRate() const;
        int getChannels() const;
        SoundFileMetadata getMetadata() const;

        /**
         * Get the sample data of the loaded file. The data is a vector of
         * floats in the range -1.0 to 1.0, and the channels are interleaved.
         * @return the sample data of the loaded file
         */
        std::vector<float> getSamples() const;

    private:
        int sampleRate;
        int channels;
        SoundFileMetadata metadata;
        std::vector<float> samples;
        bool loadMp3(std::string path);
};
