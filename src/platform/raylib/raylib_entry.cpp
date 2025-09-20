//
// Created by madoka on 25-9-16.
//


#include "Madokawaii/platform/core.h"

int main() {
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