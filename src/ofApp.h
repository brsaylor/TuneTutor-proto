#pragma once

#include "ofMain.h"
#include "ofxUI.h"

class ofApp : public ofBaseApp {

	public:
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
		
        ofxUICanvas *topGui;   	
        ofxUICanvas *midGui;
        ofxUIScrollableCanvas *markTable;
        void guiEvent(ofxUIEventArgs &e);

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

        // Sound file
        std::vector<float> inputSamples;
        int sampleRate;
        int channels;
};
