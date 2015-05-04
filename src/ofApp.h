#pragma once

#include <rubberband/RubberBandStretcher.h>
#include "ofMain.h"
#include "ofxUI.h"

#include "soundfile.h"

extern "C" {
#include <aubio/aubio.h>
}

enum PlayMode {
    PLAYMODE_PLAY_SELECTION,
    PLAYMODE_LOOP_SELECTION,
    PLAYMODE_PLAY_TO_END
};

struct Mark {

    /** Position of the mark in samples */
    int position;

    std::string label;

    ofxUILabelButton *positionButton;
    ofxUILabelToggle *selectStartToggle;
    ofxUILabelToggle *selectEndToggle;
    ofxUITextInput *labelInput;
};

struct MarkCompare {
    bool operator() (const Mark *mark1, const Mark *mark2) const {
        return mark1->position < mark2->position;
    }
};

class ofApp : public ofBaseApp {

	public:
		void setup();
		void update();
		void draw();

        void setFilePath(std::string path);

        void drawVisualization();
        void drawPitchLines();
        void drawPositionBar();

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
        void exit();

        ofSoundStream soundStream;
		
        ofxUICanvas *topGui;   	
        ofxUICanvas *midGui;
        ofxUICanvas *metadataTable;
        ofxUIScrollableCanvas *markTable;
        void guiEvent(ofxUIEventArgs &e);

        ofImage pauseImage;
        ofImage playImage;

    private:
        const std::string fontFile = "DroidSans.ttf";
        const float markWidth = 16;
        const float markHeight = 12;
        const int selectionHandleRadius = 8;
        const float pitchRangeMin = 35; // G below middle C
        const float pitchRangeMax = 86; // high D
        const float defaultSamplesPerPixel = 100;
        const float positionBarHeight = 8;
        const float positionHandleRadius = 10;

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

        float padding;

        float markStripTop;
        float markStripBottom;
        std::set<Mark*, MarkCompare> marks;
        Mark *markBeingDragged;
        Mark *getMarkAtDisplayX(int x);
        Mark *insertMark(int position, std::string label = "");
        void deleteMark(Mark *mark);
        void updateMarkPosition(Mark *mark, int position);
        ofxUILabelButton *lastMarkPositionButton;

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

        float midGuiY;

        void configureCanvas(ofxUICanvas *canvas);

        ofxUILabelButton *openFileButton;
        ofxUIImageButton *playButton;
        ofxUIRadio *playModeRadio;
        std::vector<ofxUIToggle *> playModeToggles;
        ofxUISlider *zoomSlider;
        ofxUIRangeSlider *pitchRangeSlider;
        ofxUIIntSlider *speedSlider;
        ofxUIIntSlider *transposeSlider;
        ofxUIIntSlider *tuningSlider;
        ofxUILabelButton *addMarkButton;

        // Audio setup
        bool playing;
        int bufferSize;
        void playPause();

        // Sound file
        SoundFile soundFile;
        std::vector<float> inputSamples;
        int sampleRate;
        int channels;

        int prevPlayheadPos; // Position of playhead when dragging started
        int playheadPos;

        // Time stretcher
        RubberBand::RubberBandStretcher *stretcher;
        //
        std::vector<float*> stretchInBuf;
        std::vector<float> stretchInBufL;
        std::vector<float> stretchInBufR;
        //
        std::vector<float*> stretchOutBuf;
        std::vector<float> stretchOutBufL;
        std::vector<float> stretchOutBufR;

        // Pitch detection
        aubio_pitch_t *pitchDetector;
        size_t pdBufSize;
        size_t pdHopSize;
        fvec_t *pdInBuf;
        fvec_t *pdOutBuf;
        std::vector<float> pitchValues;
        void detectPitches();
        float minPitch;
        float maxPitch;

        // Pitch visualization
        float samplesPerPixel;
        float pxPerPitchValue;
        int pitchValuesToDraw;

        std::string getSettingsPath();
        void loadSettings();
        void saveSettings();

        std::string formatTime(int sample);
};
