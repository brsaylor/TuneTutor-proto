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

#include <vector>
#include <iostream>

#include <rubberband/RubberBandStretcher.h>

#include "soundfile.h"

namespace TuneTutor {

class TimeStretcher {

    public:

        /** @param soundFile Must already have a sound loaded via load() */
        TimeStretcher(const SoundFile &soundFile);
        ~TimeStretcher();

        /** @param position the frame to seek to */
        void seek(int position);

        /**
         * Get the current playhead position as the frame index into the input
         * audio. It represents the start of the next buffer that will be fed
         * into the time stretcher.
         *
         * @return the current playhead position
         */
        int getPosition() const;

        /**
         * Set the playback speed ratio. 1 is the original speed, 0.5 is half
         * speed, etc.
         *
         * @param ratio the ratio of playback speed
         */
        void setSpeed(double ratio);

        /** @param semitones number of semitones by which to transpose */
        void setPitch(double semitones);

        /**
         * Get a block of output frames from the time stretcher and advance the
         * playhead position. The samples in each frame will be interleaved by
         * channel. The number of channels is assumed to match the number of
         * channels in the sound file.
         *
         * @param output a pointer to the output buffer
         * @param bufferSize the number of frames in the output buffer
         */
        void getOutput(float *output, int bufferSize);

    private:
        const int maxProcessSize = 512;
        const double minSpeedRatio = 0.01;

        SoundFile soundFile;
        int channels;
        std::vector<float> inputSamples;
        RubberBand::RubberBandStretcher *rubberband = NULL;
        int playheadPos;
        
        // Input buffers for the RubberBandStretcher
        std::vector<float*> stretchInBuf;
        std::vector<float> stretchInBufL;
        std::vector<float> stretchInBufR;

        // Output buffers for the RubberBandStretcher
        std::vector<float*> stretchOutBuf;
        std::vector<float> stretchOutBufL;
        std::vector<float> stretchOutBufR;
};

}
