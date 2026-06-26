//
// Direct2D backend process entry.
//

#include <objbase.h>

#include "Madokawaii/platform/core.h"

int main(int, char**) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    void** appstate = new void*;
    int retval = AppInit(*appstate);
    if (!retval) {
        delete appstate;
        CoUninitialize();
        return retval;
    }

    while (true) {
        retval = AppIterate(*appstate);
        if (!retval) break;
    }

    AppExit(*appstate);
    delete appstate;

    CoUninitialize();
    return 0;
}
