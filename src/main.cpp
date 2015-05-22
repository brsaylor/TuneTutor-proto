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
