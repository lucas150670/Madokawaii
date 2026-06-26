//
// Created by madoka on 25-9-11.
//

#ifndef MADOKAWAII_LOG_H
#define MADOKAWAII_LOG_H

namespace Madokawaii::Platform::Log {
    enum class TraceLogLevel {
        LOG_ALL = 0,        // Display all logs
        LOG_TRACE,          // Trace logging, intended for internal use only
        LOG_DEBUG,          // Debug logging, used for internal debugging, it should be disabled on release builds
        LOG_INFO,           // Info logging, used for program execution info
        LOG_WARNING,        // Warning logging, used on recoverable failures
        LOG_ERROR,          // Error logging, used on unrecoverable failures
        LOG_FATAL,          // Fatal logging, used to abort program: exit(EXIT_FAILURE)
        LOG_NONE           // Disable logging
    };
    void TraceLog(TraceLogLevel loglevel, const char *format, ...);

}

#endif //MADOKAWAII_LOG_H
