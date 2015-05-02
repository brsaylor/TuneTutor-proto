#include "util.h"

#include <string>

extern "C" {
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
}

// See https://stackoverflow.com/questions/2910377/get-home-directory-in-linux-c
std::string getHomeDirectory() {
    const char *homedir;

    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }

    return std::string(homedir);
}

