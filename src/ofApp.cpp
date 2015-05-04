#include <algorithm>
#include <cstdlib>
#include <cstdio>

#include "ofxXmlSettings.h"

#include "ofApp.h"
#include "soundfile.h"
#include "util.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetVerticalSync(true); 
    ofEnableSmoothing();
    ofSetFrameRate(60);

    // Initialize state variables
    fileName = "";
    playbackDelay = 0.0;
    zoom = 1.0;
    speed = 100;
    transpose = 0;
    tuning = 0;
    playing = false;
    playMode = PLAYMODE_PLAY_TO_END;

    markBeingDragged = nullptr;

    // Set up audio
    // FIXME: need to update channels and samplerate when soundfile is loaded
    bufferSize = 512;
    sampleRate = 44100;
    channels = 2;
    soundStream.setup(this, channels, 0, sampleRate, bufferSize, 4);
    soundStream.stop();

    // Set up time stretching
    
    stretchInBufL.resize(bufferSize);
    stretchInBufR.resize(bufferSize);
    stretchInBuf.resize(channels);
    stretchInBuf[0] = &(stretchInBufL[0]);
    stretchInBuf[1] = &(stretchInBufR[0]);

    stretchOutBufL.resize(bufferSize);
    stretchOutBufR.resize(bufferSize);
    stretchOutBuf.resize(channels);
    stretchOutBuf[0] = &(stretchOutBufL[0]);
    stretchOutBuf[1] = &(stretchOutBufR[0]);
    
    stretcher = new RubberBand::RubberBandStretcher(sampleRate, channels,
            RubberBand::RubberBandStretcher::DefaultOptions |
            RubberBand::RubberBandStretcher::OptionProcessRealTime);
    stretcher->setMaxProcessSize(bufferSize);

    /****************************
     * Set up pitch detector
     ****************************/
    
    // These are not used by aubio, just for postprocessing detected pitches
    minPitch = pitchRangeMin;
    maxPitch = pitchRangeMax;

    // Set up pitch detection
    pdBufSize = 2048;
    pdHopSize = 512;
    pitchDetector = new_aubio_pitch(const_cast<char *>("yinfft"),
            pdBufSize, pdHopSize, sampleRate);
    aubio_pitch_set_unit(pitchDetector, const_cast<char *>("midi"));
    pdInBuf = new_fvec(pdHopSize);
    pdOutBuf = new_fvec(1);

    // pitch visualization
    setSamplesPerPixel(defaultSamplesPerPixel);

    /*********************************
     * Set up GUI
     *********************************/

    draggingSelectionStart = false;
    draggingSelectionEnd = false;
    draggingViz = false;
    draggingPosition = false;

    padding = 10;

    topGui = new ofxUICanvas();
    configureCanvas(topGui);
    topGui->setGlobalButtonDimension(64);
    
    topGui->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    openFileButton = topGui->addLabelButton("Open File", false);

    topGui->addSpacer(padding, 0);

    topGui->addImageButton("back", "images/back.png", false);
    playButton = topGui->addImageButton("play", "images/play.png", false);
    playImage = *playButton->getImage();
    pauseImage.loadImage("images/pause.png");
    topGui->addImageButton("forward", "images/forward.png", false);

    topGui->addSpacer(padding, 0);

    topGui->setGlobalButtonDimension(18);
    vector<string> playModes;
    playModes.push_back("Play Selection");
    playModes.push_back("Loop Selection");
    playModes.push_back("Play to End");
    playModeRadio = topGui->addRadio("playMode", playModes,
            OFX_UI_ORIENTATION_VERTICAL);
    playModeToggles = playModeRadio->getToggles();
    playModeRadio->activateToggle("Play to End");
    playMode = PLAYMODE_PLAY_TO_END;

    topGui->addSpacer(padding, 0);

    topGui->addSlider("Playback Delay", 0.0, 2.0, &playbackDelay);
    zoomSlider = topGui->addSlider("Zoom", 0.25, 4.0, &zoom);
    topGui->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    pitchRangeSlider = topGui->addRangeSlider("Pitch Range",
            pitchRangeMin, pitchRangeMax, &minPitch, &maxPitch);
    pitchRangeSlider->getRect()->setX(zoomSlider->getRect()->getX());

    topGui->autoSizeToFitWidgets();
    ofAddListener(topGui->newGUIEvent, this, &ofApp::guiEvent);

    //----------------------------------------------------

    markStripTop = topGui->getRect()->getHeight() + padding;
    markStripBottom = markStripTop + markHeight;

    selectionStripTop = markStripTop + markHeight + 1;
    selectionStripHeight = 16;
    selectionStripBottom = selectionStripTop + selectionStripHeight;
    selectionStart = -1;
    selectionEnd = -1;

    mainColor.set(128);
    playLineColor.set(0, 255, 0);
    markLineColor.set(60, 160, 70);
    pitchLineColor.set(108, 108, 255, 128);

    vizTop = selectionStripBottom + 1;
    vizHeight = 240;
    vizBottom = vizTop + vizHeight;

    positionBarTop = vizBottom + padding;
    positionHandleY = positionBarTop + positionBarHeight/2;

    //----------------------------------------------------------

    midGuiY = positionBarTop + positionBarHeight + padding;
    midGui = new ofxUICanvas(0, midGuiY, 10, 10);
    configureCanvas(midGui);

    midGui->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    speedSlider = midGui->addIntSlider(
            "Speed (%)", 10, 100, &speed, ofGetWidth()/3 - padding, 16);
    transposeSlider = midGui->addIntSlider(
            "Transpose (semitones)", -12, 12, &transpose,
            ofGetWidth()/3 - padding, 16);
    tuningSlider = midGui->addIntSlider(
            "Tuning (cents)", -50, 50, &tuning, ofGetWidth()/3 - padding, 16);

    midGui->autoSizeToFitWidgets();
    ofAddListener(midGui->newGUIEvent, this, &ofApp::guiEvent);

    //----------------------------------------------------------
    
    float markTableGuiY = midGuiY + midGui->getRect()->getHeight() + padding;
    ofxUICanvas *markTableGui = new ofxUICanvas();
    markTableGui->getRect()->setY(markTableGuiY);
    configureCanvas(markTableGui);
    markTableGui->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    markTableGui->addLabel("Marks", OFX_UI_FONT_LARGE);
    addMarkButton = markTableGui->addLabelButton("Add Mark", false, false);
    markTableGui->autoSizeToFitWidgets();
    
    float markTableHeaderY = markTableGuiY +
        markTableGui->getRect()->getHeight() + padding; 
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
    lastMarkPositionButton = nullptr;
    ofAddListener(markTable->newGUIEvent, this, &ofApp::guiEventMarkTable);

    //-------------------------------------------------------
    
    float metadataTableX = markTable->getRect()->getX() +
        markTable->getRect()->getWidth() + padding;
    float metadataTableY = markTableHeaderY;
    metadataTable = new ofxUICanvas();
    ofxUIRectangle *rect = metadataTable->getRect();
    rect->setX(metadataTableX);
    rect->setY(metadataTableY);
    rect->setWidth(ofGetWidth() - metadataTableX);
    rect->setHeight(ofGetHeight() - metadataTableY);
    configureCanvas(metadataTable);

    ofxUILabel *label;
    ofxUITextInput *textInput;

    label = metadataTable->addLabel("titleLabel", "Title", OFX_UI_FONT_MEDIUM);
    textInput = new ofxUITextInput("title", "test title", 400, 0, 0, 0);
    metadataInputs.insert(textInput);
    metadataTable->addWidgetEastOf(textInput, "titleLabel", false);
    textInput->getRect()->setX(150);

    label = new ofxUILabel("artistLabel", "Artist", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "titleLabel", false);
    textInput = new ofxUITextInput("artist", "test artist", 400, 0, 0, 0);
    metadataInputs.insert(textInput);
    metadataTable->addWidgetEastOf(textInput, "artistLabel", false);
    textInput->getRect()->setX(150);

    label = new ofxUILabel("albumLabel", "Album", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "artistLabel", false);
    textInput = new ofxUITextInput("album", "test album", 400, 0, 0, 0);
    metadataInputs.insert(textInput);
    metadataTable->addWidgetEastOf(textInput, "albumLabel", false);
    textInput->getRect()->setX(150);

    label = new ofxUILabel("rhythmLabel", "Rhythm", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "albumLabel", false);
    textInput = new ofxUITextInput("rhythm", "test rhythm", 200, 0, 0, 0);
    metadataInputs.insert(textInput);
    metadataTable->addWidgetEastOf(textInput, "rhythmLabel", false);
    textInput->getRect()->setX(150);

    label = new ofxUILabel("keyLabel", "Key", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "rhythmLabel", false);
    textInput = new ofxUITextInput("key", "test key", 200, 0, 0, 0);
    metadataInputs.insert(textInput);
    metadataTable->addWidgetEastOf(textInput, "keyLabel", false);
    textInput->getRect()->setX(150);

    label = new ofxUILabel("tempoLabel", "Tempo", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "keyLabel", false);
    textInput = new ofxUITextInput("temp", "test tempo", 200, 0, 0, 0);
    metadataInputs.insert(textInput);
    metadataTable->addWidgetEastOf(textInput, "tempoLabel", false);
    textInput->getRect()->setX(150);

    metadataTable->autoSizeToFitWidgets();

    ofAddListener(metadataTable->newGUIEvent, this, &ofApp::guiEvent);

    if (filePath != "") {
        openFile();
    }
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

    displayStartSample = getSampleIndexFromDisplayX(padding);
    displayEndSample = getSampleIndexFromDisplayX(ofGetWidth() - padding);
    
    // Calculate screen coordinates of selection, and decide whether it is on or
    // off screen
    selectionStartX = getDisplayXFromSampleIndex(selectionStart);
    selectionEndX = getDisplayXFromSampleIndex(selectionEnd);
    if (selectionStartX > ofGetWidth() - padding || selectionEndX < padding) {
        drawSelection = false;
    } else {
        drawSelection = true;
        if (selectionStartX < padding) {
            selectionStartX = padding;
        }
        if (selectionEndX > ofGetWidth() - padding) {
            selectionEndX = ofGetWidth() - padding;
        }
    }

    positionHandleX = playheadPos / (float) (inputSamples.size() / channels)
        * (ofGetWidth() - 2 * padding)
        + padding;
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(255);
    ofSetColor(mainColor);

    ofFill();

    // Draw selection strip
    if (drawSelection) {
        ofRect(
                selectionStartX, selectionStripTop,
                selectionEndX - selectionStartX, selectionStripHeight);
    }


    // Draw visualization area
    drawVisualization();
    drawPitchLines();

    // Draw playhead line
    ofSetColor(playLineColor);
    ofLine(
            ofGetWidth() * .5, selectionStripTop, 
            ofGetWidth() * .5, vizBottom);
    
    // Draw marks
    ofSetColor(markLineColor);
    float markX;
    for (Mark *mark : marks) {
        if (mark->position < displayStartSample) {
            continue;
        } else if (mark->position > displayEndSample) {
            break;
        }

        markX = getDisplayXFromSampleIndex(mark->position);
        ofTriangle(
                markX - markWidth * .5, markStripTop,
                markX + markWidth * .5, markStripTop,
                markX, markStripBottom);
        ofLine(markX, markStripBottom, markX, vizBottom);
    }

    drawPositionBar();
}

void ofApp::drawVisualization() {
    float top = selectionStripBottom;
    float width = ofGetWidth() - 2 * padding;
    float height = vizHeight;

    ofFill();
    ofSetColor(96);
    ofRect(padding, top, width, height);

    ofSetColor(255);

    ofBeginShape();
    for (int i = 0; i < pitchValuesToDraw; i++) {
        // subtract pitchValuesToDraw/2 to put the playhead position in the middle
        int pitchIndex = (playheadPos / pdHopSize + i) - pitchValuesToDraw/2;
        if (pitchIndex > pitchValues.size()) {
            break;
        }
        float pitch;
        if (pitchIndex < 0) {
           pitch = 0.;
        } else {
           pitch = pitchValues[pitchIndex];
        }
        float x = i * pxPerPitchValue + padding;
        float y = ((pitch + transpose + tuning / 100.0 - minPitch)
                / (maxPitch - minPitch) * -1 + 1) * height + top;
            
        ofVertex(x, y);
    }
    ofVertex(width + padding, top + height);
    ofVertex(padding, top + height);
    ofEndShape();
}

void ofApp::drawPitchLines() {
    float y;
    ofSetColor(pitchLineColor);
    for (float pitch = minPitch; pitch < maxPitch; pitch++) {
        y = ((pitch - minPitch) / (maxPitch - minPitch) * -1 + 1)
            * vizHeight + vizTop;
        ofLine(padding, y, ofGetWidth() - padding, y);
    }
}

void ofApp::drawPositionBar() {
    ofSetColor(mainColor);
    ofRect(
            padding, positionBarTop,
            ofGetWidth() - 2 * padding, positionBarHeight);

    // Draw the position indicator handle
    ofSetColor(markLineColor);
    ofCircle(positionHandleX, positionHandleY, positionHandleRadius);
}

//--------------------------------------------------------------
void ofApp::guiEvent(ofxUIEventArgs &e) {
    std::string name = e.widget->getName();

    if (e.widget == openFileButton && openFileButton->getValue()) {
		ofFileDialogResult openFileResult = ofSystemLoadDialog(
                "Open Sound File"); 
		if (openFileResult.bSuccess) {
            filePath = openFileResult.getPath();
            openFile();
		}
    } else if (e.widget == playButton) {
        if (playButton->getValue()) {
            playPause();
        }
    } else if (e.widget == playModeToggles[0]) {
        playMode = PLAYMODE_PLAY_SELECTION;
    } else if (e.widget == playModeToggles[1]) {
        playMode = PLAYMODE_LOOP_SELECTION;
    } else if (e.widget == playModeToggles[2]) {
        playMode = PLAYMODE_PLAY_TO_END;
    } else if (e.widget == (ofxUIWidget *) speedSlider) {
        stretcher->setTimeRatio(1.0 / (speed / 100.0));
    } else if (e.widget == (ofxUIWidget *) zoomSlider) {
        setSamplesPerPixel(defaultSamplesPerPixel / zoom);
    } else if (e.widget == (ofxUIWidget *) transposeSlider
            || e.widget == (ofxUIWidget *) tuningSlider) {
        stretcher->setPitchScale(pow(2.0, (transpose + tuning / 100.0) / 12.0));
    } else if (e.widget == (ofxUIWidget *) pitchRangeSlider) {
        // There is no integer range slider, so round off the values here
        minPitch = (int) minPitch;
        maxPitch = (int) maxPitch;
        pitchRangeSlider->setValueLow(minPitch);
        pitchRangeSlider->setValueHigh(maxPitch);
    } else if (e.widget->getKind() == OFX_UI_WIDGET_TEXTINPUT) {
        ofxUITextInput *input = (ofxUITextInput *) e.widget;
        if (input->getInputTriggerType() == OFX_UI_TEXTINPUT_ON_ENTER) {
        } else if (input->getInputTriggerType() == OFX_UI_TEXTINPUT_ON_FOCUS) {
            ofLog() << "metadata table input focused";
            // Need to manually unfocus the mark text inputs, because they
            // are in a different canvas and don't know that this one got focus.
            for (Mark *mark : marks) {
                mark->labelInput->setFocus(false);
            }
        } else if(input->getInputTriggerType() == OFX_UI_TEXTINPUT_ON_UNFOCUS) {
        }
    }
}

void ofApp::guiEventMarkTable(ofxUIEventArgs &e) {
    if (e.widget->getKind() == OFX_UI_WIDGET_TEXTINPUT) {
        ofxUITextInput *input = (ofxUITextInput *) e.widget;
        if (input->getInputTriggerType() == OFX_UI_TEXTINPUT_ON_ENTER) {
        } else if (input->getInputTriggerType() == OFX_UI_TEXTINPUT_ON_FOCUS) {
            ofLog() << "mark table input focused";
            // Need to manually unfocus the metadata text inputs, because they
            // are in a different canvas and don't know that this one got focus.
            for (ofxUITextInput *otherInput : metadataInputs) {
                otherInput->setFocus(false);
            }
            // Also, manually unfocus the other mark text inputs.
            // TODO: Figure out why they aren't automatically unfocused
            for (Mark *mark : marks) {
                if (mark->labelInput != input) {
                    mark->labelInput->setFocus(false);
                }
            }
        } else if(input->getInputTriggerType() == OFX_UI_TEXTINPUT_ON_UNFOCUS) {
            // Find the mark associated with this label and update the label
            for (Mark *mark : marks) {
                if (mark->labelInput == input) {
                    mark->label = input->getTextString();
                    break;
                }
            }
        }
    }
}

/**
 * Play or pause playback, depending on whether currently playing
 */
void ofApp::playPause() {
    if (playing) {
        playButton->setImage(&playImage);
        playing = false;
        soundStream.stop();
    } else {
        playButton->setImage(&pauseImage);
        playing = true;
        soundStream.start();
    }
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
void ofApp::mousePressed(int x, int y, int button) {
    if (button == 0) {

        // Left click on mark strip
        if (y >= markStripTop && y <= markStripBottom) {
            Mark *mark = getMarkAtDisplayX(x);
            if (mark == nullptr) {
                insertMark(getSampleIndexFromDisplayX(x));
            } else {
                markBeingDragged = mark;
            }

        // Left click on selection strip
        } else if (y >= selectionStripTop && y <= selectionStripBottom) {
            if (x > selectionStartX - selectionHandleRadius &&
                    x < selectionStartX + selectionHandleRadius) {
                // Start dragging the selection start
                draggingSelectionStart = true;
            } else if (x > selectionEndX - selectionHandleRadius &&
                    x < selectionEndX + selectionHandleRadius) {
                // Start dragging the selection end
                draggingSelectionEnd = true;
            } else {
                // Place the selection
                selectionStart = getSampleIndexFromDisplayX(x - 100);
                selectionEnd = getSampleIndexFromDisplayX(x + 100);
            }

        // Left click on visualization
        } else if (y >= vizTop && y <= vizBottom) {
            draggingViz = true;
            vizDragStartX = x;
            prevPlayheadPos = playheadPos;
            if (playing) {
                soundStream.stop();
            }

        // Left click on position handle
        } else if (y >= positionHandleY - positionHandleRadius
                && y <= positionHandleY + positionHandleRadius
                && x >= positionHandleX - positionHandleRadius
                && x <= positionHandleX + positionHandleRadius) {
            draggingPosition = true;
            positionDragStartX = x;
            prevPlayheadPos = playheadPos;
            if (playing) {
                soundStream.stop();
            }
        }

    } else if (button == 2) {
        if (y >= markStripTop && y <= markStripBottom) {
            // Check for right-click on mark
            Mark *clickedMark = getMarkAtDisplayX(x);
            if (clickedMark != nullptr) {
                ofLog() << "Mark at position " << clickedMark->position
                    << " clicked";
                deleteMark(clickedMark);
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
    if (draggingSelectionStart) {
        selectionStart = getSampleIndexFromDisplayX(x);
    } else if (draggingSelectionEnd) {
        selectionEnd = getSampleIndexFromDisplayX(x);
    } else if (draggingViz) {
        playheadPos = prevPlayheadPos + (vizDragStartX - x) * samplesPerPixel;
    } else if (draggingPosition) {
        playheadPos = prevPlayheadPos - (positionDragStartX - x) *
            (inputSamples.size() / channels / (ofGetWidth() - 2 * padding));
    } else if (markBeingDragged != nullptr) {
        updateMarkPosition(markBeingDragged, getSampleIndexFromDisplayX(x));
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
    if (draggingViz || draggingPosition) {
        draggingViz = false;
        draggingPosition = false;
        if (playing) {
            soundStream.start();
        }
    }
    draggingSelectionStart = false;
    draggingSelectionEnd = false;
    markBeingDragged = nullptr;
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

//--------------------------------------------------------------
void ofApp::audioOut(float *output, int bufferSize, int nChannels) {
    /*
     * This code just plays the sound
    for (int i = 0; i < bufferSize; i++){
        output[i*nChannels] = inputSamples[playheadPos*nChannels + i*nChannels];
        output[i*nChannels + 1] = inputSamples[playheadPos*nChannels + i*nChannels + 1];
    }
    playheadPos += bufferSize;
    */

    // While there are fewer than bufferSize output samples available, feed more
    // input samples into the timestretcher
    // Note: buffer size of stretcher input doesn't necessarily need to be the
    // same as the output buffer size
    while (stretcher->available() < bufferSize) {
    
        // Deinterleave into the stretcher input buffers
        for (int i = 0; i < bufferSize; i++) {
            stretchInBufL[i] = inputSamples[(playheadPos + i) * channels];
            stretchInBufR[i] = inputSamples[(playheadPos + i) * channels + 1];
        }

        // FIXME: check for end of sound
        
        stretcher->process(&(stretchInBuf[0]), bufferSize, false);

        playheadPos += bufferSize;
        if (playheadPos > selectionEnd) {
            if (playMode == PLAYMODE_LOOP_SELECTION) {
                playheadPos = selectionStart;
            } else if (playMode == PLAYMODE_PLAY_SELECTION) {
                playheadPos = selectionStart;
                playPause();
            }
        }
    }

    size_t samplesRetrieved = stretcher->retrieve(&(stretchOutBuf[0]), bufferSize);
    if (samplesRetrieved != (size_t) bufferSize) {
        ofLog() << "Retrieved " << samplesRetrieved << endl;
    }

    // Interleave output from stretcher into audio output
    for (int i = 0; i < bufferSize; i++) {
        output[i * nChannels] = stretchOutBufL[i];
        output[i * nChannels + 1] = stretchOutBufR[i];
    }
}

void ofApp::setFilePath(std::string path) {
    filePath = path;
}

/** Depends on filePath being set. */
bool ofApp::openFile() {
    bool ok = soundFile.load(filePath);
    if (ok) {
        fileName = ofFilePath::getBaseName(filePath);
        ofLog() << "fileName = " << fileName;
        sampleRate = soundFile.getSampleRate();
        channels = soundFile.getChannels();
        inputSamples = soundFile.getSamples();

        ofLog() << "Successfully opened file "
            << filePath
            << "\nSize: " << inputSamples.size()
            << "\nSample rate: " << sampleRate
            << "\nChannels: " << channels;

        SoundFileMetadata metadata = soundFile.getMetadata();
        ((ofxUITextInput *) (metadataTable->getWidget("title")))
            ->setTextString(metadata.title);
        ((ofxUITextInput *) (metadataTable->getWidget("artist")))
            ->setTextString(metadata.artist);
        ((ofxUITextInput *) (metadataTable->getWidget("album")))
            ->setTextString(metadata.album);

        loadSettings();
        detectPitches();
    } else {
        ofLogError() << "Error opening sound file";
    }
    return ok;
}

//-------------------------------------------------------
void ofApp::detectPitches() {

    pitchValues.resize(inputSamples.size() / pdHopSize);

    static int spuriousHold = 0;

    ofLog() << "Detecting pitches..."; 

    // Hop
    for (size_t i = 0; i < pitchValues.size(); i++) {

        // Fill input buffer by summing the channels of a chunk of the audio
        for (size_t j = 0; j < pdHopSize; j++) {
            if ((i * pdHopSize + j) * channels + 1 > inputSamples.size()) {
                break;
            }
            pdInBuf->data[j] =
                inputSamples[(i * pdHopSize + j) * channels] +
                inputSamples[(i * pdHopSize + j) * channels + 1];
        }

        // Detect the pitch for this hop
        aubio_pitch_do(pitchDetector, pdInBuf, pdOutBuf);

        float pitch = pdOutBuf->data[0];
        
        // Post-process to remove spurious pitch estimates
        if (i > 0 && spuriousHold < 10 &&
                (aubio_pitch_get_confidence(pitchDetector) < 0.50
                 || (pitchValues[i - 1] - pitch) > 7)) {
            pitchValues[i] = pitchValues[i - 1];
            spuriousHold++;
        } else {
            spuriousHold = 0;
            pitchValues[i] = pitch;
        }
    }

    ofLog() << "done." << endl;
}

float ofApp::getDisplayXFromSampleIndex(int sampleIndex) {
   return ofGetWidth() / 2 + (sampleIndex - playheadPos) / samplesPerPixel;
}

int ofApp::getSampleIndexFromDisplayX(float displayX) {
    return samplesPerPixel * (displayX - ofGetWidth() / 2) + playheadPos;
}

Mark *ofApp::getMarkAtDisplayX(int x) {
    Mark *locatedMark = nullptr;
    float markX;
    for (Mark *mark : marks) {
        if (mark->position < displayStartSample) {
            continue;
        } else if (mark->position > displayEndSample) {
            break;
        }
        markX = getDisplayXFromSampleIndex(mark->position);
        if (x >= markX - markWidth && x <= markX + markWidth) {
            locatedMark = mark;
            break;
        }
    }
    return locatedMark;
}

Mark *ofApp::insertMark(int position, std::string label) {
    Mark *mark = new Mark();
    mark->position = position;
    mark->label = label;

    // Append a row of widgets to the mark table
    mark->positionButton =  new ofxUILabelButton(
            formatTime(mark->position), false);
    if (lastMarkPositionButton == nullptr) {
        markTable->addWidgetPosition(mark->positionButton,
                OFX_UI_WIDGET_POSITION_RIGHT, OFX_UI_ALIGN_LEFT);
    } else {
        markTable->addWidgetSouthOf(mark->positionButton,
                lastMarkPositionButton->getName(), false);
    }
    lastMarkPositionButton = mark->positionButton;
    mark->selectStartToggle = new ofxUILabelToggle(
            "", false, 20, 0, 0, 0, OFX_UI_FONT_MEDIUM);
    markTable->addWidgetPosition(mark->selectStartToggle,
            OFX_UI_WIDGET_POSITION_RIGHT, OFX_UI_ALIGN_LEFT);
    mark->selectStartToggle->getRect()->setX(100);

    mark->selectEndToggle = new ofxUILabelToggle(
            "", false, 20, 0, 0, 0, OFX_UI_FONT_MEDIUM);
    markTable->addWidgetPosition(mark->selectEndToggle,
            OFX_UI_WIDGET_POSITION_RIGHT, OFX_UI_ALIGN_LEFT);
    mark->selectEndToggle->getRect()->setX(200);

    mark->labelInput = new ofxUITextInput("", mark->label, 100, 0, 0, 0);
    markTable->addWidgetPosition(mark->labelInput,
            OFX_UI_WIDGET_POSITION_RIGHT, OFX_UI_ALIGN_LEFT);
    mark->labelInput->getRect()->setX(300);

    marks.insert(mark);

    return mark;
}

void ofApp::deleteMark(Mark *mark) {
    marks.erase(mark);
    delete mark;
}

// Because marks is an ordered set, and the key for ordering is the position,
// the set needs to be updated when a mark's position changes.
void ofApp::updateMarkPosition(Mark *mark, int position) {
    marks.erase(mark);
    mark->position = position;
    marks.insert(mark);
}

std::string ofApp::getSettingsPath() {
    return getHomeDirectory() + "/.TuneTutor/metadata/" + fileName;
}

void ofApp::loadSettings() {
    ofLog() << "Loading settings";
    std::string path = getSettingsPath();
    topGui->loadSettings(path + "/settings1.xml");
    midGui->loadSettings(path + "/settings2.xml");
    metadataTable->loadSettings(path + "/metadata.xml");

    ofxXmlSettings xml;
    if (xml.loadFile(path + "/marks.xml")) {
        xml.pushTag("marks");
        int numMarks = xml.getNumTags("mark");
        for (int i = 0; i < numMarks; i++) {
            xml.pushTag("mark", i);
            insertMark(xml.getValue("position", 0), xml.getValue("label", ""));
            xml.popTag();
        }
        xml.popTag();
        selectionStart = xml.getValue("selectionStart", -1);
        selectionEnd = xml.getValue("selectionEnd", -1);
    }
}

void ofApp::saveSettings() {
    ofLog() << "Saving settings";
    std::string path = getSettingsPath();
    ofDirectory dir(path);
    if (!dir.exists()) {
        dir.create(true);
    }
    topGui->saveSettings(path + "/settings1.xml");
    midGui->saveSettings(path + "/settings2.xml");
    metadataTable->saveSettings(path + "/metadata.xml");

    ofxXmlSettings xml;
    xml.addTag("marks");
    xml.pushTag("marks");
    int i = 0;
    for (Mark *mark : marks) {
        xml.addTag("mark");
        xml.pushTag("mark", i);
        xml.addValue("position", mark->position);

        // Workaround for when text input has been typed into but hasn't been
        // unfocused
        mark->label = mark->labelInput->getTextString();

        xml.addValue("label", mark->label);
        xml.popTag();
        i++;
    }
    xml.popTag();
    xml.setValue("selectionStart", selectionStart);
    xml.setValue("selectionEnd", selectionEnd);
    xml.save(path + "/marks.xml");
}

void ofApp::setSamplesPerPixel(float ratio) {
    samplesPerPixel = ratio;
    pxPerPitchValue = pdHopSize / samplesPerPixel;
    pitchValuesToDraw = ofGetWidth() / pxPerPitchValue;
}

std::string ofApp::formatTime(int sample) {
    int milliseconds = sample / (float) sampleRate * 1000;
    int seconds = milliseconds / 1000;
    milliseconds -= seconds * 1000;
    int minutes = seconds / 60;
    seconds -= minutes * 60;
    char buffer[10];
    snprintf(buffer, 10, "%d:%02d.%03d", minutes, seconds, milliseconds);
    return std::string(buffer);
}

void ofApp::exit() {
    saveSettings();
}
