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
