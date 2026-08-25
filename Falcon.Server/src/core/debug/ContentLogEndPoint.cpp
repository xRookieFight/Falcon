#include "core/debug/ContentLogEndPoint.h"

#include <cstdio>

#ifndef _WIN32

#include <unistd.h>

#endif

ContentLogEndPoint::ContentLogEndPoint(bool useColor) : mUseColor(useColor) {
#ifndef _WIN32
    if (mUseColor && isatty(fileno(stdout)) == 0)
        mUseColor = false;
#endif
}

const char *ContentLogEndPoint::_getColorCode(LogLevel level) {
    switch (level) {
        case LogLevel::Verbose:
            return "\033[90m";
        case LogLevel::Warning:
            return "\033[33m";
        case LogLevel::Error:
            return "\033[31m";
        case LogLevel::Fatal:
            return "\033[1;31m";
        default:
            return "\033[37m";
    }
}

void ContentLogEndPoint::log(const LogDetails &details) {
    std::lock_guard<std::mutex> guard(mMutex);

    if (mUseColor) {
        fprintf(stdout, "%s[%s %s]\033[0m %s\n", _getColorCode(details.mLevel), details.getTimestamp().c_str(),
                toString(details.mLevel), details.mMessage.c_str());
    } else {
        fprintf(stdout, "[%s %s] %s\n", details.getTimestamp().c_str(), toString(details.mLevel),
                details.mMessage.c_str());
    }

    fflush(stdout);
}

void ContentLogEndPoint::flush() {
    std::lock_guard<std::mutex> guard(mMutex);
    fflush(stdout);
}
