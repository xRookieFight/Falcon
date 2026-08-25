#pragma once

#include "core/debug/ILogEndPoint.h"

#include <mutex>

class ContentLogEndPoint : public ILogEndPoint {
public:
    explicit ContentLogEndPoint(bool useColor = true);

    void log(const LogDetails &details) override;

    void flush() override;

private:
    static const char *_getColorCode(LogLevel level);

    bool mUseColor;
    std::mutex mMutex;
};
