#include <algorithm>
#include <cmath>

#include "timestretcher.h"

namespace TuneTutor {

TimeStretcher::TimeStretcher(const SoundFile &soundFile) {
    this->soundFile = soundFile;
    channels = soundFile.getChannels();
    inputSamples = soundFile.getSamples();

    stretchInBufL.resize(maxProcessSize);
    stretchInBufR.resize(maxProcessSize);
    stretchInBuf.resize(channels);
    stretchInBuf[0] = &(stretchInBufL[0]);
    stretchInBuf[1] = &(stretchInBufR[0]);

    stretchOutBufL.resize(maxProcessSize);
    stretchOutBufR.resize(maxProcessSize);
    stretchOutBuf.resize(channels);
    stretchOutBuf[0] = &(stretchOutBufL[0]);
    stretchOutBuf[1] = &(stretchOutBufR[0]);

    rubberband = new RubberBand::RubberBandStretcher(
            soundFile.getSampleRate(), channels,
            RubberBand::RubberBandStretcher::DefaultOptions |
            RubberBand::RubberBandStretcher::OptionProcessRealTime);
    rubberband->setMaxProcessSize(maxProcessSize);

    playheadPos = 0;
}

void TimeStretcher::seek(int position) {
    playheadPos = position;
}

int TimeStretcher::getPosition() const {
    return playheadPos;
}

void TimeStretcher::setSpeed(double ratio) {
    rubberband->setTimeRatio(1.0 / std::max(ratio, minSpeedRatio));
}

void TimeStretcher::setPitch(double semitones) {
    rubberband->setPitchScale(std::pow(2.0, semitones / 12.0));
}

void TimeStretcher::getOutput(float *output, int bufferSize) {
 
    // While there are fewer than bufferSize output samples available, feed more
    // input samples into the time rubberband
    while (rubberband->available() < bufferSize) {
    
        // Deinterleave into the rubberband input buffers
        int i, j;
        for (i = 0, j = (playheadPos + i) * channels;
                i < maxProcessSize && j < inputSamples.size();
                i++, j += channels) {
            stretchInBufL[i] = inputSamples[j];
            stretchInBufR[i] = inputSamples[j + 1];
        }
        while (i < maxProcessSize) {
            stretchInBufL[i] = 0;
            stretchInBufR[i] = 0;
            i++;
        }

        rubberband->process(&(stretchInBuf[0]), maxProcessSize, false);

        playheadPos += maxProcessSize;
    }

    size_t samplesRetrieved = rubberband->retrieve(&(stretchOutBuf[0]),
            bufferSize);

    // Interleave output from rubberband into audio output
    for (int i = 0; i < samplesRetrieved && i < bufferSize; i++) {
        output[i * channels] = stretchOutBufL[i];
        output[i * channels + 1] = stretchOutBufR[i];
    }
}

TimeStretcher::~TimeStretcher() {
    if (rubberband != NULL) {
        delete rubberband;
    }
}

}
