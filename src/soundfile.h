/* Copyright 2015 Benjamin R. Saylor <brsaylor@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include <vector>

namespace TuneTutor {

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
        bool isLoaded() const;
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
        bool loaded;
};

}
