#include "pitchdetector.h"

namespace TuneTutor {

PitchDetector::PitchDetector(const SoundFile &soundFile) {
    aubioPitchDetector = new_aubio_pitch(const_cast<char *>("yinfft"),
            bufferSize, hopSize, soundFile.getSampleRate()); 
    aubio_pitch_set_unit(aubioPitchDetector, const_cast<char *>("midi"));
    inputBuffer = new_fvec(hopSize);
    outputBuffer = new_fvec(1);
    inputSamples = soundFile.getSamples();
    channels = soundFile.getChannels();
}

void PitchDetector::detectPitches() {
    pitches.resize(inputSamples.size() / hopSize);

    static int spuriousHold = 0;

    // Hop
    for (size_t i = 0; i < pitches.size(); i++) {

        // Fill input buffer by summing the channels of a chunk of the audio
        for (size_t j = 0; j < hopSize; j++) {
            if ((i * hopSize + j) * channels + 1 > inputSamples.size()) {
                break;
            }
            inputBuffer->data[j] =
                    inputSamples[(i * hopSize + j) * channels] +
                    inputSamples[(i * hopSize + j) * channels + 1];
        }

        // Detect the pitch for this hop
        aubio_pitch_do(aubioPitchDetector, inputBuffer, outputBuffer);

        float pitch = outputBuffer->data[0];
        
        // Post-process to remove spurious pitch estimates
        // TODO: is this helpful? Needs to be tweaked
        if (i > 0 && spuriousHold < 10 &&
                (aubio_pitch_get_confidence(aubioPitchDetector) < 0.50
                 || (pitches[i - 1] - pitch) > 7)) {
            pitches[i] = pitches[i - 1];
            spuriousHold++;
        } else {
            spuriousHold = 0;
            pitches[i] = pitch;
        }
    }
}

int PitchDetector::getSampleInterval() const {
    return hopSize;
}

const std::vector<float> & PitchDetector::getPitches() const {
    return pitches;
}

PitchDetector::~PitchDetector() {
    del_aubio_pitch(aubioPitchDetector);
    del_fvec(inputBuffer);
    del_fvec(outputBuffer);
}

}
