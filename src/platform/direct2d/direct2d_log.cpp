//
// Direct2D backend logging.
//

#include "Madokawaii/platform/log.h"

#include <cstdarg>
#include <cstdio>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace Madokawaii::Platform::Log {
    namespace {
        const char* Prefix(TraceLogLevel level) {
            switch (level) {
            case TraceLogLevel::LOG_TRACE: return "TRACE";
            case TraceLogLevel::LOG_DEBUG: return "DEBUG";
            case TraceLogLevel::LOG_INFO: return "INFO";
            case TraceLogLevel::LOG_WARNING: return "WARN";
            case TraceLogLevel::LOG_ERROR: return "ERROR";
            case TraceLogLevel::LOG_FATAL: return "FATAL";
            case TraceLogLevel::LOG_ALL: return "ALL";
            case TraceLogLevel::LOG_NONE: return "NONE";
            default: return "INFO";
            }
        }
    }

    void TraceLog(TraceLogLevel loglevel, const char* format, ...) {
        char message[2048]{};
        va_list args;
        va_start(args, format);
        vsnprintf(message, sizeof(message), format, args);
        va_end(args);

        char line[2304]{};
        snprintf(line, sizeof(line), "%s: %s\n", Prefix(loglevel), message);
        fputs(line, stdout);
        OutputDebugStringA(line);
    }
}
