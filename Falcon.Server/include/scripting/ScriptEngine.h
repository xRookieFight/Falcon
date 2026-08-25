#pragma once

#include <cstdint>
#include <memory>
#include <string>

struct JSRuntime;
struct JSContext;
class ScriptApi;
class ServerNetworkHandler;
class ServerActor;

class ScriptEngine {
public:
    static const int64_t DEFAULT_WATCHDOG_MS = 2000;

    ScriptEngine();

    ~ScriptEngine();

    void bindHost(ServerNetworkHandler &host);

    ScriptEngine(const ScriptEngine &) = delete;

    ScriptEngine &operator=(const ScriptEngine &) = delete;

    bool isReady() const { return mContext != nullptr; }

    bool evaluate(const std::string &source, const std::string &name, bool asModule = false);

    bool evaluateFile(const std::string &path);

    void tick(int64_t currentTick);

    void onProjectileHitBlock(ServerActor &projectile, int32_t x, int32_t y, int32_t z);

    void setWatchdogMs(int64_t milliseconds) { mWatchdogMs = milliseconds; }

    JSContext *context() { return mContext; }

    JSRuntime *runtime() { return mRuntime; }

private:
    void _installConsole();

    void _pumpJobs();

    bool _checkException(bool hadError, const std::string &name);

    friend int _scriptInterruptHandler(JSRuntime *runtime, void *opaque);

    JSRuntime *mRuntime;
    JSContext *mContext;
    std::unique_ptr<ScriptApi> mApi;
    int64_t mWatchdogMs;
    int64_t mExecutionDeadlineMs;
};
