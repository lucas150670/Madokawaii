//
// Created by madoka on 25-9-16.
//


#include <cstdio>
#include <cstring>
#include <raylib.h>

#include "Madokawaii/platform/core.h"

char gVendor[256] = {0};
char gRenderer[256] = {0};
char gVersion[256] = {0};

void LogCallback(int msgType, const char *text, va_list args)
{
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);

    if (strstr(buffer, "Vendor:")) {
        strcpy(gVendor, buffer + strlen("    > Vendor:   "));
    }
    if (strstr(buffer, "Renderer:")) {
        strcpy(gRenderer, buffer + strlen("    > Renderer: "));
    }
    if (strstr(buffer, "Version:")) {
        strcpy(gVersion, buffer + strlen("    > Version:  "));
    }

    switch (msgType)
    {
        case LOG_INFO: printf("INFO:  "); break;
        case LOG_WARNING: printf("WARN:  "); break;
        case LOG_ERROR: printf("ERROR: "); break;
        case LOG_DEBUG: printf("DEBUG: "); break;
        default: break;
    }
    printf("%s\n", buffer);
}

int main() {
    SetTraceLogCallback(LogCallback);

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