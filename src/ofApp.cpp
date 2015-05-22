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

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ofxXmlSettings.h"

#include "ofApp.h"
#include "soundfile.h"
#include "util.h"

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetVerticalSync(true); 
    ofEnableSmoothing();
    ofSetFrameRate(60);
    
#if __APPLE__ && TARGET_OS_MAC
    ofSetDataPathRoot("../Resources/");
#endif

    // Initialize state variables
    fileName = "";
    playbackDelay = 0.0;
    zoom = 1.0;
    speed = 100;
    transpose = 0;
    tuning = 0;
    playing = false;
    playbackDelayed = false;
    silentSamplesPlayed = 0;
    playMode = PLAYMODE_PLAY_TO_END;

    markBeingDragged = NULL;

    // Set up audio
    bufferSize = 512;
    sampleRate = 44100;
    channels = 2;
    soundStream.setup(this, channels, 0, sampleRate, bufferSize, 4);
    soundStream.stop();

    stretcher = NULL;

    minPitch = pitchRangeMin;
    maxPitch = pitchRangeMax;

    pitchDetector = NULL;
    pitchesDetected = false;

    /*********************************
     * Set up GUI
     *********************************/

    draggingSelectionStart = false;
    draggingSelectionEnd = false;
    draggingViz = false;
    draggingPosition = false;

    topGui = new ofxUICanvas();
    configureCanvas(topGui);
    topGui->setGlobalButtonDimension(64);
    
    topGui->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    openFileButton = topGui->addLabelButton("Open File", false);

    topGui->addSpacer(padding, 0);

    backButton = topGui->addImageButton("back", "images/back.png", false);
    playButton = topGui->addImageButton("play", "images/play.png", false);
    playImage = *playButton->getImage();
    pauseImage.loadImage("images/pause.png");
    forwardButton = topGui->addImageButton(
            "forward", "images/forward.png", false);

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
    topGui->getRect()->setWidth(ofGetWidth());
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
    pitchLineHighlightColor.set(108, 108, 255, 200);
    highlightPitches.insert(55.0);
    highlightPitches.insert(62.0);
    highlightPitches.insert(69.0);
    highlightPitches.insert(76.0);

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
            "Speed (%)", 10, 100, &speed, ofGetWidth()/3 - padding, 24);
    transposeSlider = midGui->addIntSlider(
            "Transpose (semitones)", -12, 12, &transpose,
            ofGetWidth()/3 - padding, 24);
    tuningSlider = midGui->addIntSlider(
            "Tuning (cents)", -50, 50, &tuning, ofGetWidth()/3 - padding, 24);

    midGui->autoSizeToFitWidgets();
    midGui->getRect()->setWidth(ofGetWidth());
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
    ofAddListener(markTableGui->newGUIEvent, this, &ofApp::guiEventMarkTable);
    
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
    lastMarkPositionButton = NULL;
    ofAddListener(markTable->newGUIEvent, this, &ofApp::guiEventMarkTable);

    markTable->getSRect()->setWidth(ofGetWidth() / 2 - padding / 2);
    markTable->getSRect()->setHeight(ofGetHeight() - markTableY);
    markTableGui->getRect()->setWidth(markTable->getSRect()->getWidth());
    markTableHeader->getRect()->setWidth(markTable->getSRect()->getWidth());

    //-------------------------------------------------------
    
    metadataTable = new ofxUICanvas();
    metadataTable->setWidgetSpacing(10);
    float metadataTableX = ofGetWidth() / 2 + padding / 2;
    float metadataTableY = markTableGuiY;
    ofxUIRectangle *rect = metadataTable->getRect();
    rect->setX(metadataTableX);
    rect->setY(metadataTableY);
    rect->setWidth(ofGetWidth() - metadataTableX);
    rect->setHeight(ofGetHeight() - metadataTableY);
    configureCanvas(metadataTable);

    ofxUILabel *label;
    ofxUITextInput *textInput;

    label = metadataTable->addLabel("infoHeaderLabel", "Tune Information",
            OFX_UI_FONT_LARGE);

    label = metadataTable->addLabel("titleLabel", "Title", OFX_UI_FONT_MEDIUM);
    textInput = new ofxUITextInput("title", "", 400, 0, 0, 0);
    metadataInputs.insert(textInput);
    metadataTable->addWidgetEastOf(textInput, "titleLabel", false);
    textInput->getRect()->setX(150);

    label = new ofxUILabel("artistLabel", "Artist", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "titleLabel", false);
    textInput = new ofxUITextInput("artist", "", 400, 0, 0, 0);
    metadataInputs.insert(textInput);
    metadataTable->addWidgetEastOf(textInput, "artistLabel", false);
    textInput->getRect()->setX(150);

    label = new ofxUILabel("albumLabel", "Album", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "artistLabel", false);
    textInput = new ofxUITextInput("album", "", 400, 0, 0, 0);
    metadataInputs.insert(textInput);
    metadataTable->addWidgetEastOf(textInput, "albumLabel", false);
    textInput->getRect()->setX(150);

    label = new ofxUILabel("rhythmLabel", "Rhythm", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "albumLabel", false);
    textInput = new ofxUITextInput("rhythm", "", 200, 0, 0, 0);
    metadataInputs.insert(textInput);
    metadataTable->addWidgetEastOf(textInput, "rhythmLabel", false);
    textInput->getRect()->setX(150);

    label = new ofxUILabel("keyLabel", "Key", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "rhythmLabel", false);
    textInput = new ofxUITextInput("key", "", 200, 0, 0, 0);
    metadataInputs.insert(textInput);
    metadataTable->addWidgetEastOf(textInput, "keyLabel", false);
    textInput->getRect()->setX(150);

    label = new ofxUILabel("tempoLabel", "Tempo", OFX_UI_FONT_MEDIUM);
    metadataTable->addWidgetSouthOf(label, "keyLabel", false);
    textInput = new ofxUITextInput("temp", "", 200, 0, 0, 0);
    metadataInputs.insert(textInput);
    metadataTable->addWidgetEastOf(textInput, "tempoLabel", false);
    textInput->getRect()->setX(150);

    metadataTable->autoSizeToFitWidgets();
    rect->setHeight(ofGetHeight() - metadataTableY);

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
    //canvas->setColorBack(ofColor(128));
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

    // Enforce a minimum visible pitch range of 5 semitones
    if (maxPitch - minPitch < 5) {
        if (maxPitch >= pitchRangeMax) {
            minPitch = maxPitch - 5;
            pitchRangeSlider->setValueLow(minPitch);
        } else {
            maxPitch = minPitch + 5;
            pitchRangeSlider->setValueHigh(maxPitch);
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(190);
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

    // Draw playhead line
    ofSetColor(playLineColor);
    ofLine(
            ofGetWidth() * .5, selectionStripTop, 
            ofGetWidth() * .5, vizBottom);
    

    drawPositionBar();
}

void ofApp::drawVisualization() {
    float top = selectionStripBottom;
    float width = ofGetWidth() - 2 * padding;
    float height = vizHeight;
    
    ofFill();
    ofSetColor(96);
    ofRect(padding, top, width, height);

    if (!pitchesDetected) {
        return;
    }
    vector<float> pitchValues = pitchDetector->getPitches();

    ofSetColor(255);

    ofBeginShape();
    for (int i = 0; i < pitchValuesToDraw; i++) {
        // subtract pitchValuesToDraw/2 to put the playhead position in the middle
        int pitchIndex = (playheadPos / pitchDetector->getSampleInterval() + i)
                - pitchValuesToDraw/2;
        if (pitchIndex > (int) pitchValues.size()) {
            break;
        }
        float pitch;
        if (pitchIndex < 0) {
           pitch = minPitch;
        } else {
           pitch = max(pitchValues[pitchIndex], minPitch);
           pitch = min(pitch, maxPitch);
        }
        float x = i * pxPerPitchValue + padding;
        float y = ((pitch + transpose + tuning / 100.0 - minPitch)
                / (maxPitch - minPitch) * -1 + 1) * height + top;
            
        ofVertex(x, y);
    }
    ofVertex(width + padding, top + height); // bottom right corner
    ofVertex(padding, top + height); // bottom left corner
    ofEndShape();
}

void ofApp::drawPitchLines() {
    float y;
    ofSetColor(pitchLineColor);
    for (float pitch = minPitch; pitch < maxPitch; pitch++) {
        y = ((pitch - minPitch) / (maxPitch - minPitch) * -1 + 1)
            * vizHeight + vizTop;
        if (highlightPitches.count(pitch)) {
            ofSetColor(pitchLineHighlightColor);
        } else {
            ofSetColor(pitchLineColor);
        }
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
    } else if (e.widget == backButton && backButton->getValue()) {
        seekToNextMark(true);
    } else if (e.widget == forwardButton && forwardButton->getValue()) {
        seekToNextMark(false);
    } else if (e.widget == playModeToggles[0]) {
        playMode = PLAYMODE_PLAY_SELECTION;
    } else if (e.widget == playModeToggles[1]) {
        playMode = PLAYMODE_LOOP_SELECTION;
    } else if (e.widget == playModeToggles[2]) {
        playMode = PLAYMODE_PLAY_TO_END;
    } else if (e.widget == (ofxUIWidget *) speedSlider) {
        if (stretcher != NULL) {
            stretcher->setSpeed(speed / 100.0);
        }
    } else if (e.widget == (ofxUIWidget *) zoomSlider) {
        setSamplesPerPixel(defaultSamplesPerPixel / zoom);
    } else if (e.widget == (ofxUIWidget *) transposeSlider
            || e.widget == (ofxUIWidget *) tuningSlider) {
        if (stretcher != NULL) {
            stretcher->setPitch(transpose + tuning / 100.0);
        }
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
    } else if (e.widget == addMarkButton && addMarkButton->getValue()) {
        insertMark(playheadPos, "");
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
        playbackDelayed = true;
        silentSamplesPlayed = 0;
        soundStream.start();
    }
}

void ofApp::seekToNextMark(bool backward) {
    Mark m; // Dummy mark containing playhead position, for comparison to marks
    m.position = playheadPos;
    std::set<Mark*, MarkCompare>::iterator it;
    if (backward) {
        it = marks.lower_bound(&m);
        if (it == marks.begin() || marks.empty()) {
            // The playhead position is before the first mark,
            // so just seek to the beginning of the audio
            seek(0);
        } else {
            it--;
            seek((*it)->position);
        }
    } else {
        // Seek forward
        it = marks.upper_bound(&m);
        if (it == marks.end()) {
            // No mark forward of the playhead position, so seek to the end
            seek(inputSamples.size() / channels - 1);
        } else {
            seek((*it)->position);
        }
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
            if (mark == NULL) {
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
        } else if (y >= vizTop && y <= vizBottom && stretcher != NULL) {
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
                && x <= positionHandleX + positionHandleRadius
                && stretcher != NULL
                ) {
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
            if (clickedMark != NULL) {
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
        seek(prevPlayheadPos + (vizDragStartX - x) * samplesPerPixel);
    } else if (draggingPosition) {
        seek(prevPlayheadPos - (positionDragStartX - x) *
            (inputSamples.size() / channels / (ofGetWidth() - 2 * padding)));
    } else if (markBeingDragged != NULL) {
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
    markBeingDragged = NULL;
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

    if (playheadPos * channels >= inputSamples.size()) {
        playheadPos = inputSamples.size() / channels;
        playPause();
    }

    if (stretcher == NULL) {
        return;
    }

    if (playbackDelayed) {
        memset((void *) output, 0, bufferSize * channels * sizeof(float));
        silentSamplesPlayed += bufferSize;
        if (silentSamplesPlayed / (float) sampleRate >= playbackDelay) {
            playbackDelayed = false;
        }
        return;
    }

    stretcher->getOutput(output, bufferSize);
    playheadPos = stretcher->getPosition();

    if (playheadPos > selectionEnd) {
        if (playMode == PLAYMODE_LOOP_SELECTION) {
            seek(selectionStart);
            playbackDelayed = true;
            silentSamplesPlayed = 0;
        } else if (playMode == PLAYMODE_PLAY_SELECTION) {
            seek(selectionStart);
            playPause();
        }
    }
}

void ofApp::seek(int position) {
    if (position <= 0) {
        playheadPos = 0;
    } else if (position * channels >= inputSamples.size()) {
        playheadPos = inputSamples.size() / channels - 1;
    } else {
        playheadPos = position;
    }
    stretcher->seek(playheadPos);
}

void ofApp::setFilePath(std::string path) {
    filePath = path;
}

/** Depends on filePath being set. */
bool ofApp::openFile() {
    if (soundFile.isLoaded()) {
        saveSettings();
    }
    if (playing) {
        playPause();
    }
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

        clearMarks();
        clearMetadata();

        TuneTutor::SoundFileMetadata metadata = soundFile.getMetadata();
        ((ofxUITextInput *) (metadataTable->getWidget("title")))
            ->setTextString(metadata.title);
        ((ofxUITextInput *) (metadataTable->getWidget("artist")))
            ->setTextString(metadata.artist);
        ((ofxUITextInput *) (metadataTable->getWidget("album")))
            ->setTextString(metadata.album);

        if (stretcher != NULL) {
            delete stretcher;
        }
        stretcher = new TuneTutor::TimeStretcher(soundFile);
        seek(0);
        if (pitchDetector != NULL) {
            delete pitchDetector;
        }
        pitchDetector = new TuneTutor::PitchDetector(soundFile);
        pitchDetector->detectPitches();
        pitchesDetected = true;
        setSamplesPerPixel(defaultSamplesPerPixel);
        loadSettings();
    } else {
        ofLogError() << "Error opening sound file";
    }
    return ok;
}

float ofApp::getDisplayXFromSampleIndex(int sampleIndex) {
   return ofGetWidth() / 2 + (sampleIndex - playheadPos) / samplesPerPixel;
}

int ofApp::getSampleIndexFromDisplayX(float displayX) {
    return samplesPerPixel * (displayX - ofGetWidth() / 2) + playheadPos;
}

Mark *ofApp::getMarkAtDisplayX(int x) {
    Mark *locatedMark = NULL;
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

    // Refuse to insert a mark at the same position as another mark
    std::set<Mark*, MarkCompare>::iterator it = marks.lower_bound(mark);
    if (it != marks.end()
            && (*(marks.lower_bound(mark)))->position == position) {
        ofLog() << "insertMark: not inserting, because a mark is already there"
            << std::endl;
        return NULL;
    }

    // Append a row of widgets to the mark table
    mark->positionButton =  new ofxUILabelButton(
            formatTime(mark->position), false);
    if (lastMarkPositionButton == NULL) {
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

void ofApp::clearMarks() {
    for (Mark *mark : marks) {
        delete mark;
    }
    marks.clear();
    markTable->clearWidgets();
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

void ofApp::clearMetadata() {
    for (ofxUITextInput *input : metadataInputs) {
        input->setTextString("");
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
    if (pitchDetector == NULL) {
        return;
    }
    samplesPerPixel = ratio;
    pxPerPitchValue = pitchDetector->getSampleInterval() / samplesPerPixel;
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
