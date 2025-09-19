//
// Created by madoka on 2025/9/19.
//
#include <raylib.h>
#include <cstdarg>
#include <cstdio>
#include "Madokawaii/platform/log.h"

namespace Madokawaii::Platform::Log {

    static int ToRayLevel(TraceLogLevel lvl) {
        switch (lvl) {
            case TraceLogLevel::LOG_ALL: return LOG_ALL;
            case TraceLogLevel::LOG_TRACE: return LOG_TRACE;
            case TraceLogLevel::LOG_DEBUG: return LOG_DEBUG;
            case TraceLogLevel::LOG_INFO: return LOG_INFO;
            case TraceLogLevel::LOG_WARNING: return LOG_WARNING;
            case TraceLogLevel::LOG_ERROR: return LOG_ERROR;
            case TraceLogLevel::LOG_FATAL: return LOG_FATAL;
            case TraceLogLevel::LOG_NONE: return LOG_NONE;
            default: return LOG_INFO;
        }
    }

    void TraceLog(TraceLogLevel loglevel, const char *format, ...) {
        char buffer[2048];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        ::TraceLog(ToRayLevel(loglevel), "%s", buffer);
    }
}