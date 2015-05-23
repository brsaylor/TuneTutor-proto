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

/*
 * Main header file for the TuneTutor openFrameworks app
 */

#pragma once

#include "ofMain.h"
#include "ofxUI.h"

#include "soundfile.h"
#include "timestretcher.h"
#include "pitchdetector.h"

enum PlayMode {
    PLAYMODE_PLAY_SELECTION,
    PLAYMODE_LOOP_SELECTION,
    PLAYMODE_PLAY_TO_END
};

/**
 * Represents a position on the timeline marked by the user.
 */
struct Mark {

    /** Position of the mark in sample frames */
    int position;

    /** Description of the mark entered by the user */
    std::string label;

    // ofxUI widgets that appear in the mark table (lower left area of the GUI)
    // are here so they can be updated when the mark changes.
    ofxUILabelButton *positionButton;
    ofxUILabelButton *selectStartToggle;
    ofxUILabelButton *selectEndToggle;
    ofxUITextInput *labelInput;
};

/**
 * Comparison function object used for the std::set of of Marks
 */
struct MarkCompare {
    bool operator() (const Mark *mark1, const Mark *mark2) const {
        return mark1->position < mark2->position;
    }
};

/**
 * TuneTutor main class
 */
class ofApp : public ofBaseApp {

	public:

        // openFrameworks event handlers
		void setup();
		void update();
		void draw();
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
        void audioOut(float* output, int bufferSize, int nChannels);
        void guiEvent(ofxUIEventArgs &e);
        void guiEventMarkTable(ofxUIEventArgs &e);
        void exit();

        void setFilePath(std::string path);

    private:
        const std::string fontFile = "DroidSans.ttf";
        const float padding = 6;
        const float markWidth = 16;
        const float markHeight = 12;
        const int selectionHandleRadius = 8;
        const float pitchRangeMin = 55; // G below middle C
        const float pitchRangeMax = 86; // high D
        std::set<float> highlightPitches;
        const float defaultSamplesPerPixel = 100;
        const float positionBarHeight = 8;
        const float positionHandleRadius = 10;

        ofSoundStream soundStream;

        void drawVisualization();
        void drawPitchLines();
        void drawPositionBar();
		
        // ofxUI stuff
        ofxUICanvas *topGui;   	
        ofxUICanvas *midGui;
        ofxUICanvas *metadataTable;
        ofxUIScrollableCanvas *markTable;
        float midGuiY;
        ofxUILabelButton *openFileButton;
        ofxUIImageButton *playButton;
        ofxUIImageButton *forwardButton;
        ofxUIImageButton *backButton;
        ofxUIRadio *playModeRadio;
        std::vector<ofxUIToggle *> playModeToggles;
        ofxUISlider *zoomSlider;
        ofxUIRangeSlider *pitchRangeSlider;
        ofxUIIntSlider *speedSlider;
        ofxUIIntSlider *transposeSlider;
        ofxUIIntSlider *tuningSlider;
        ofxUILabelButton *addMarkButton;
        ofxUILabelButton *lastMarkPositionButton;
        std::set<ofxUITextInput *> metadataInputs;
        void clearMetadata();
        void configureCanvas(ofxUICanvas *canvas);

        ofImage pauseImage;
        ofImage playImage;

        void setSamplesPerPixel(float ratio);

        std::string filePath;
        std::string fileName;
        bool openFile();

        float playbackDelay;
        float zoom;
        int speed;
        int transpose;
        int tuning;
        PlayMode playMode;

        int displayStartSample;
        int displayEndSample;
        float getDisplayXFromSampleIndex(int sampleIndex);
        int getSampleIndexFromDisplayX(float displayX);

        float markStripTop;
        float markStripBottom;
        std::set<Mark*, MarkCompare> marks;
        Mark *markBeingDragged;
        Mark *getMarkAtDisplayX(int x);
        Mark *insertMark(int position, std::string label = "");
        void deleteMark(Mark *mark);
        void clearMarks();
        void updateMarkPosition(Mark *mark, int position);

        bool drawSelection;
        float selectionStripTop;
        float selectionStripHeight;
        float selectionStripBottom;
        int selectionStart;
        int selectionEnd;
        float selectionStartX;
        float selectionEndX;
        bool draggingSelectionStart;
        bool draggingSelectionEnd;
        bool selectionDragStartX;

        ofColor mainColor;
        ofColor playLineColor;
        ofColor markLineColor;
        ofColor pitchLineColor;
        ofColor pitchLineHighlightColor;

        float vizTop;
        float vizHeight;
        float vizBottom;
        bool draggingViz;
        int vizDragStartX;

        float positionBarTop;
        float positionHandleY;
        float positionHandleX;
        bool draggingPosition;
        int positionDragStartX;

        // Audio setup
        bool playing;
        bool playbackDelayed; // true if in a playback delay state
        int bufferSize;
        void playPause();

        /** Number of samples of silence played since playback delay started */
        int silentSamplesPlayed;

        // Sound file
        TuneTutor::SoundFile soundFile;
        std::vector<float> inputSamples;
        int sampleRate;
        int channels;

        // Playhead
        int prevPlayheadPos; // Position of playhead when dragging started
        int playheadPos;
        void seek(int position); // Set playhead position
        void seekToNextMark(bool backward);

        // Time stretcher
        TuneTutor::TimeStretcher *stretcher;

        // Pitch detection
        TuneTutor::PitchDetector *pitchDetector;
        float minPitch;
        float maxPitch;
        bool pitchesDetected;

        // Pitch visualization
        float samplesPerPixel;
        float pxPerPitchValue;
        int pitchValuesToDraw;

        std::string getSettingsPath();
        void loadSettings();
        void saveSettings();

        std::string formatTime(int sample);
};
