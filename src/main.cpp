#include <string>
#include "ofMain.h"
#include "ofApp.h"

int main(int argc, char *argv[]) {
    ofSetupOpenGL(1100, 700, OF_WINDOW);
    ofApp *app = new ofApp();
    if (argc > 1) {
        app->setFilePath(std::string(argv[1]));
    } else {
        app->setFilePath("");
    }
    ofRunApp(app);
}
