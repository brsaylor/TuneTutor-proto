#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetVerticalSync(true); 
    ofEnableSmoothing();
    ofSetFrameRate(60);

    // Initialize state variables
    playbackDelay = 0.0;
    zoom = 1.0;
    speed = 100;
    transpose = 0;
    tuning = 0;

    padding = 10;

    topGui = new ofxUICanvas();
    configureCanvas(topGui);
    topGui->setGlobalButtonDimension(64);
    
    topGui->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    topGui->addLabelButton("Open File", false);

    topGui->addSpacer(padding, 0);

    topGui->addImageButton("back", "images/back.png", false);
    topGui->addImageButton("play", "images/play.png", false);
    topGui->addImageButton("forward", "images/forward.png", false);

    topGui->addSpacer(padding, 0);

    topGui->setGlobalButtonDimension(18);
    vector<string> playModes;
    playModes.push_back("Play Selection");
    playModes.push_back("Loop Selection");
    playModes.push_back("Play to End");
    ofxUIRadio *radio = topGui->addRadio("playMode", playModes,
            OFX_UI_ORIENTATION_VERTICAL);
    radio->activateToggle("Play Selection");

    topGui->addSpacer(padding, 0);

    topGui->addSlider("Playback Delay", 0.0, 2.0, &playbackDelay);
    topGui->addSlider("Zoom", 0.25, 4.0, &zoom);

    topGui->autoSizeToFitWidgets();
    ofAddListener(topGui->newGUIEvent, this, &ofApp::guiEvent);

    //----------------------------------------------------

    markStripY = topGui->getRect()->getHeight() + padding;
    markWidth = 16;
    markHeight = 12;
    marks.push_back(100);
    marks.push_back(250);
    marks.push_back(650);

    selectionStripY = markStripY + markHeight;
    selectionStripHeight = 16;
    selectionStart = marks[0];
    selectionEnd = marks[2];

    mainColor.set(128);
    playLineColor.set(0, 255, 0);
    markLineColor.set(60, 160, 70);

    vizHeight = 240;
    vizBottom = selectionStripY + selectionStripHeight + vizHeight;

    contextBoxY = vizBottom + padding;
    contextBoxHeight = 28;
    contextStripHeight = 16;
    contextStripY = contextBoxY + contextBoxHeight/2 - contextStripHeight/2;

    //----------------------------------------------------------

    midGuiY = contextBoxY + contextBoxHeight + padding;
    midGui = new ofxUICanvas(0, midGuiY, 10, 10);
    configureCanvas(midGui);

    midGui->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    midGui->addIntSlider("Speed (%)", 10, 100, &speed, ofGetWidth()/3 - padding, 16);
    midGui->addIntSlider("Transpose (semitones)", -12, 12, &transpose, ofGetWidth()/3 - padding, 16);
    midGui->addIntSlider("Tuning (cents)", -50, 50, &tuning, ofGetWidth()/3 - padding, 16);

    midGui->autoSizeToFitWidgets();
    ofAddListener(midGui->newGUIEvent, this, &ofApp::guiEvent);

    //----------------------------------------------------------
    
    float markTableHeaderY = midGuiY + midGui->getRect()->getHeight() + padding; 
    ofxUICanvas *markTableHeader = new ofxUICanvas(0, markTableHeaderY, 10, 10);
    configureCanvas(markTableHeader);
    markTableHeader->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    ofxUILabel *headerLabel;
    headerLabel = markTableHeader->addLabel("Time", OFX_UI_FONT_MEDIUM);
    headerLabel = markTableHeader->addLabel("Start Sel.", OFX_UI_FONT_MEDIUM);
    headerLabel->getRect()->setX(100);
    headerLabel = markTableHeader->addLabel("End Sel.", OFX_UI_FONT_MEDIUM);
    headerLabel->getRect()->setX(200);
    headerLabel = markTableHeader->addLabel("Description", OFX_UI_FONT_MEDIUM);
    headerLabel->getRect()->setX(300);
    markTableHeader->autoSizeToFitWidgets();

    float markTableY = markTableHeaderY +
        markTableHeader->getRect()->getHeight() + padding;
    markTable = new ofxUIScrollableCanvas(
            0, markTableY,
            ofGetWidth() / 2, ofGetHeight() - markTableY - padding);
    configureCanvas(markTable);
    markTable->setScrollableDirections(false, true);

    ofxUILabelButton *prevMarkButton = NULL;
    ofxUILabelButton *markButton = NULL;
    ofxUILabelToggle *selectStartToggle = NULL;
    ofxUILabelToggle *selectEndToggle = NULL;
    ofxUITextInput *markDescriptionInput = NULL;
    for (int i = 0; i < 15; i++) {
        markButton = new ofxUILabelButton(
                std::string("0:0") + std::to_string((i)), false);
        if (prevMarkButton == NULL) {
            markTable->addWidgetPosition(markButton,
                    OFX_UI_WIDGET_POSITION_RIGHT, OFX_UI_ALIGN_LEFT);
        } else {
            markTable->addWidgetSouthOf(markButton,
                    std::string("0:0") + std::to_string((i-1)), false);
        }
        prevMarkButton = markButton;

        selectStartToggle = new ofxUILabelToggle(
                "", false, 20, 0, 0, 0, OFX_UI_FONT_MEDIUM);
        markTable->addWidgetPosition(selectStartToggle,
                OFX_UI_WIDGET_POSITION_RIGHT, OFX_UI_ALIGN_LEFT);
        selectStartToggle->getRect()->setX(100);

        selectEndToggle = new ofxUILabelToggle(
                "", false, 20, 0, 0, 0, OFX_UI_FONT_MEDIUM);
        markTable->addWidgetPosition(selectEndToggle,
                OFX_UI_WIDGET_POSITION_RIGHT, OFX_UI_ALIGN_LEFT);
        selectEndToggle->getRect()->setX(200);

        markDescriptionInput = new ofxUITextInput("",
                std::string("Mark ") + std::to_string((i)),
                100, 0, 0, 0);
        markTable->addWidgetPosition(markDescriptionInput,
                OFX_UI_WIDGET_POSITION_RIGHT, OFX_UI_ALIGN_LEFT);
        markDescriptionInput->getRect()->setX(300);

    }
    markTable->autoSizeToFitWidgets();

    //-------------------------------------------------------
    
    float metadataTableX = markTable->getRect()->getX() +
        markTable->getRect()->getWidth() + padding;
    float metadataTableY = markTableHeaderY;
    ofxUICanvas *metadataTable = new ofxUICanvas(
            metadataTableX, metadataTableY,
            ofGetWidth() - metadataTableX,
            ofGetHeight() - metadataTableY);
    configureCanvas(metadataTable);

    ofxUILabel *label;
    ofxUITextInput *textInput;

    label = metadataTable->addLabel("Title", OFX_UI_FONT_MEDIUM);
    textInput = new ofxUITextInput("", "test title", 100, 0, 0, 0);
    metadataTable->addWidgetEastOf(textInput, "Title", false);

    label = new ofxUILabel("Artist", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "Title", false);
    textInput = new ofxUITextInput("", "test artist", 100, 0, 0, 0);
    metadataTable->addWidgetEastOf(textInput, "Artist", false);

    label = new ofxUILabel("Album", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "Artist", false);
    textInput = new ofxUITextInput("", "test album", 100, 0, 0, 0);
    metadataTable->addWidgetEastOf(textInput, "Album", false);

    label = new ofxUILabel("Rhythm", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "Album", false);
    textInput = new ofxUITextInput("", "test rhythm", 100, 0, 0, 0);
    metadataTable->addWidgetEastOf(textInput, "Rhythm", false);

    label = new ofxUILabel("Key", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "Rhythm", false);
    textInput = new ofxUITextInput("", "test key", 100, 0, 0, 0);
    metadataTable->addWidgetEastOf(textInput, "Key", false);

    label = new ofxUILabel("Tempo", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "Key", false);
    textInput = new ofxUITextInput("", "test tempo", 100, 0, 0, 0);
    metadataTable->addWidgetEastOf(textInput, "Tempo", false);

    metadataTable->autoSizeToFitWidgets();

    ofAddListener(markTable->newGUIEvent, this, &ofApp::guiEvent);
}

// Set common parameters of canvases
void ofApp::configureCanvas(ofxUICanvas *canvas) {
    canvas->setFont(fontFile);
    canvas->setFontSize(OFX_UI_FONT_LARGE, 12);
    canvas->setFontSize(OFX_UI_FONT_MEDIUM, 8);           
    canvas->setFontSize(OFX_UI_FONT_SMALL, 8);
}

//--------------------------------------------------------------
void ofApp::update() {

}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(255);
    ofSetColor(mainColor);

    ofFill();

    // Draw mark strip
    for (unsigned int i = 0; i < marks.size(); i++) {
        ofTriangle(
                marks[i] - markWidth * .5, markStripY,
                marks[i] + markWidth * .5, markStripY,
                marks[i], markStripY + markHeight);
    }

    // Draw selection strip
    ofRect(
            selectionStart, selectionStripY,
            selectionEnd - selectionStart, selectionStripHeight);

    // Draw visualization area
    ofNoFill();
    ofRect(
            padding, selectionStripY + selectionStripHeight,
            ofGetWidth() - 2 * padding, vizHeight);

    // Draw play and mark lines
    ofSetColor(playLineColor);
    ofLine(
            ofGetWidth() * .5, selectionStripY, 
            ofGetWidth() * .5, vizBottom);
    ofSetColor(markLineColor);
    for (unsigned int i = 0; i < marks.size(); i++) {
        ofLine(
                marks[i], selectionStripY,
                marks[i], vizBottom);
    }

    // Draw the context strip
    ofSetColor(mainColor);
    ofRect(
            padding, contextStripY,
            ofGetWidth() - 2 * padding, contextStripHeight);
    ofFill();
    ofRect(200, contextStripY, 120, contextStripHeight);

    // Draw the context box
    ofNoFill();
    ofRect(180, contextBoxY, 190, contextBoxHeight);
    ofSetColor(playLineColor);
    ofLine(
            180 + 190/2, contextBoxY,
            180 + 190/2, contextBoxY + contextBoxHeight);
}

//--------------------------------------------------------------
void ofApp::guiEvent(ofxUIEventArgs &e) {
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) { 

}
