/* Copyright 2015 Benjamin R. Saylor <brsaylor@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <vector>

extern "C" {
#include <aubio/aubio.h>
}

#include "soundfile.h"

namespace TuneTutor {

class PitchDetector {

    public:

        /** @param soundFile Must already have a sound loaded via load() */
        PitchDetector(const SoundFile &soundFile);
        ~PitchDetector();

        /** Run the pitch detection */
        void detectPitches();
        
        /** @return the number of input audio frames per output pitch value */
        int getSampleInterval() const;

        /**
         * Get the detected pitches, represented as MIDI note values.
         *
         * @return a vector of the detected pitches
         */
        const std::vector<float> & getPitches() const;

    private:
        const int bufferSize = 2048;
        const int hopSize = 512;

        aubio_pitch_t *aubioPitchDetector;
        fvec_t *inputBuffer;
        fvec_t *outputBuffer;
        std::vector<float> inputSamples;
        std::vector<float> pitches;
        int channels;
};

}
