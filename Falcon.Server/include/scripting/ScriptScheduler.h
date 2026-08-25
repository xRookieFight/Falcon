#pragma once

#include <cstdint>
#include <vector>

struct JSContext;

class ScriptScheduler {
public:
    explicit ScriptScheduler(JSContext *context) : mContext(context) {}

    ~ScriptScheduler();

    uint32_t run(void *callback);

    uint32_t runTimeout(void *callback, int64_t delayTicks);

    uint32_t runInterval(void *callback, int64_t periodTicks);

    void clear(uint32_t runId);

    void tick(int64_t currentTick);

    void clearAll();

private:
    struct Task {
        uint32_t mId;
        void *mCallback;
        int64_t mNextTick;
        int64_t mPeriodTicks;
        bool mRepeating;
    };

    uint32_t _add(void *callback, int64_t delayTicks, int64_t periodTicks, bool repeating);

    JSContext *mContext;
    std::vector<Task> mTasks;
    uint32_t mNextId = 1;
    int64_t mCurrentTick = 0;
    bool mInitialised = false;
};
