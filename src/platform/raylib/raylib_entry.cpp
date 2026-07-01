//
// Created by madoka on 25-9-16.
//


#include <raylib.h>

#include "Madokawaii/platform/core.hpp"
#include "raylib_runtime.hpp"

int main(int argc, char *argv[]) {
    SetTraceLogCallback(Madokawaii::Platform::Raylib::LogCallback);

    void ** appstate = new void *;
    int retval = AppInit(*appstate);
    if (!retval) {
        delete appstate;
        return retval;
    }
    while (true) {
        retval = AppIterate(*appstate);
        if (!retval) {
            break;
        }
    }
    // Clear up *appstate - AppExit do this.
    AppExit(*appstate);
    delete appstate;
    return 0;
}
