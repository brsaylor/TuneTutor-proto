#pragma once

#include <rubberband/RubberBandStretcher.h>
#include "ofMain.h"
#include "ofxUI.h"

#include "soundfile.h"

extern "C" {
#include <aubio/aubio.h>
}

class ofApp : public ofBaseApp {

	public:
		void setup();
		void update();
		void draw();

        void drawVisualization();

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
        ofSoundStream soundStream;
		
        ofxUICanvas *topGui;   	
        ofxUICanvas *midGui;
        ofxUICanvas *metadataTable;
        ofxUIScrollableCanvas *markTable;
        void guiEvent(ofxUIEventArgs &e);

        ofImage pauseImage;
        ofImage playImage;

    private:
        std::string fontFile = "DroidSans.ttf";

        float playbackDelay;
        float zoom;
        int speed;
        int transpose;
        int tuning;

        float padding;

        float markStripY;
        float markHeight;
        float markWidth;
        std::vector<float> marks;  // x coords

        float selectionStripY;
        float selectionStripHeight;
        float selectionStart;
        float selectionEnd;

        ofColor mainColor;
        ofColor playLineColor;
        ofColor markLineColor;

        float vizHeight;
        float vizBottom;

        float contextStripY;
        float contextStripHeight;
        float contextBoxY;
        float contextBoxHeight;

        float midGuiY;

        void configureCanvas(ofxUICanvas *canvas);

        ofxUILabelButton *openFileButton;
        ofxUIImageButton *playButton;
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
        int pxPerPitchValue;
        int pitchValuesToDraw;
};
