//
// Experimental GDI+ backend process entry.
//

#include "gdiplus_platform.hpp"

#include "Madokawaii/platform/core.hpp"

#include <objbase.h>

int main(int, char**) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    Madokawaii::Platform::GdiPlusBackend::Startup();

    void** appstate = new void*;
    int retval = AppInit(*appstate);
    if (!retval) {
        delete appstate;
        Madokawaii::Platform::GdiPlusBackend::Shutdown();
        CoUninitialize();
        return retval;
    }

    while (true) {
        retval = AppIterate(*appstate);
        if (!retval) break;
    }

    AppExit(*appstate);
    delete appstate;

    Madokawaii::Platform::GdiPlusBackend::Shutdown();
    CoUninitialize();
    return 0;
}
