# TuneTutor

TuneTutor is an audio player designed to help musicians learn music by ear.  It
provides independent control of the tempo and pitch of a recording, as well as
the ability to mark, select, and loop parts of the audio. It also displays a
visualization of detected pitches to aid in identifying parts of the melody.

I wrote TuneTutor for a multimedia application development class I took at San
Francisco State University in Spring 2015. It is an alpha-quality application
and I consider it a prototype for something better to come.

-- Ben Saylor (brsaylor@gmail.com)

## Library Dependencies

* openFrameworks 0.8.4 - http://openframeworks.cc/
* ofxUI - https://github.com/rezaali/ofxUI
* Rubber Band 1.8.1 - http://breakfastquay.com/rubberband/
* Aubio 0.4.0 or newer - http://aubio.org/
* libmpg123 1.16.0 or newer - http://www.mpg123.de/

## Build Instructions

### Linux

These instructions should work for Linux Mint 17.1, Ubuntu 14.04 LTS, and
similar distributions.

1. Download openFrameworks 0.8.4 from http://openframeworks.cc/download/ and
   extract the archive.

2. Follow the openFrameworks setup instructions at
   http://openframeworks.cc/setup/linux-codeblocks/ (It's probably unnecessary
   to install Codeblocks, however.)

3. Clone ofxUI from https://github.com/rezaali/ofxUI into the openFrameworks
   addons directory.

4. sudo apt-get install librubberband-dev libaubio-dev libmpg123-dev

5. Place the TuneTutor directory under apps/myApps/ in the openFrameworks
   directory tree.

6. If you don't have clang installed, you can either install it or comment out
   the PROJECT_CXX and PROJECT_CC lines in config.make to build with g++.

7. From within the TuneTutor directory, run "make".

8. To run TuneTutor, type "make run" or bin/TuneTutor.

### OS X 10.10 Yosemite

1. Download openFrameworks 0.8.4 from http://openframeworks.cc/download/ and
   extract the archive.

2. Follow the Xcode setup guide at http://openframeworks.cc/setup/xcode

3. Clone ofxUI from https://github.com/rezaali/ofxUI into the openFrameworks
   addons directory.

4. Place the TuneTutor directory under apps/myApps/ in the openFrameworks
   directory tree.

5. Download the Aubio framework (aubio-0.4.1.darwin_framework.zip) from
   http://aubio.org/download, extract it, and place aubio.framework under the
   TuneTutor directory.

6. Create a new folder under TuneTutor and name it "third-party".

7. Download mpg123 from
   http://sourceforge.net/projects/mpg123/files/mpg123/1.22.1/ and place the
   extracted folder, mpg123-1.22.1, in the third-party folder.

8. Download the Rubber Band Library v1.8.1 source from
   http://www.breakfastquay.com/rubberband/
   and place the extracted folder, rubberband-1.8.1, in the third-party folder.

9. Patch, build, and install librubberband: Open a terminal and cd into
   TuneTutor/third-party/rubberband-1.8.1, then run the following commands:
    * patch -p1 < ../../rubberband-1.8.1.patch
    * mkdir lib
    * make -f Makefile.osx library
    * sudo cp lib/librubberband.dylib /usr/lib

10. Open TuneTutor.xcodeproj in Xcode, hit the build/run button, and cross your
    fingers!
