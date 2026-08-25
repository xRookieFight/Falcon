#include "core/debug/BedrockLog.h"

#include <algorithm>
#include <cstdio>
#include <mutex>
#include <vector>

namespace BedrockLog {

    namespace {

        std::mutex &getEndPointMutex() {
            static std::mutex mutex;
            return mutex;
        }

        std::vector<std::shared_ptr<ILogEndPoint> > &getEndPoints() {
            static std::vector<std::shared_ptr<ILogEndPoint> > endPoints;
            return endPoints;
        }

        LogLevel &getMinimumLevel() {
            static LogLevel level = LogLevel::Info;
            return level;
        }

        std::string formatMessage(const char *format, va_list args) {
            va_list argsCopy;
            va_copy(argsCopy, args);

            const int length = vsnprintf(nullptr, 0, format, argsCopy);
            va_end(argsCopy);

            if (length <= 0)
                return std::string();

            std::string message((size_t) length, '\0');
            vsnprintf(&message[0], (size_t) length + 1, format, args);
            return message;
        }

    }

    void addEndPoint(const std::shared_ptr<ILogEndPoint> &endPoint) {
        if (!endPoint)
            return;

        std::lock_guard<std::mutex> guard(getEndPointMutex());

        std::vector<std::shared_ptr<ILogEndPoint> > &endPoints = getEndPoints();
        if (std::find(endPoints.begin(), endPoints.end(), endPoint) == endPoints.end())
            endPoints.push_back(endPoint);
    }

    void removeEndPoint(const std::shared_ptr<ILogEndPoint> &endPoint) {
        std::lock_guard<std::mutex> guard(getEndPointMutex());

        std::vector<std::shared_ptr<ILogEndPoint> > &endPoints = getEndPoints();
        endPoints.erase(std::remove(endPoints.begin(), endPoints.end(), endPoint), endPoints.end());
    }

    void clearEndPoints() {
        std::lock_guard<std::mutex> guard(getEndPointMutex());
        getEndPoints().clear();
    }

    void setLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> guard(getEndPointMutex());
        getMinimumLevel() = level;
    }

    LogLevel getLogLevel() {
        std::lock_guard<std::mutex> guard(getEndPointMutex());
        return getMinimumLevel();
    }

    void log_va(LogCategory category, LogRule rule, LogAreaID area, LogLevel level, const char *file, int line,
                const char *format, va_list args) {
        (void) category;

        std::vector<std::shared_ptr<ILogEndPoint> > endPoints;

        {
            std::lock_guard<std::mutex> guard(getEndPointMutex());

            if (rule != LogRule::IgnoreLevel && level < getMinimumLevel())
                return;

            endPoints = getEndPoints();
        }

        if (endPoints.empty())
            return;

        LogDetails details;
        details.mArea = area;
        details.mLevel = level;
        details.mFile = file;
        details.mLine = line;
        details.mMessage = formatMessage(format, args);

        for (const std::shared_ptr<ILogEndPoint> &endPoint: endPoints)
            endPoint->log(details);
    }

    void log(LogCategory category, LogRule rule, LogAreaID area, LogLevel level, const char *file, int line,
             const char *format, ...) {
        va_list args;
        va_start(args, format);
        log_va(category, rule, area, level, file, line, format, args);
        va_end(args);
    }

    void flush() {
        std::vector<std::shared_ptr<ILogEndPoint> > endPoints;

        {
            std::lock_guard<std::mutex> guard(getEndPointMutex());
            endPoints = getEndPoints();
        }

        for (const std::shared_ptr<ILogEndPoint> &endPoint: endPoints)
            endPoint->flush();
    }

}
