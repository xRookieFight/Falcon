#pragma once

#include "Core/Math/Vector3f.h"
#include "Core/Math/Vector3i.h"
#include "Level/Chunk.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

struct LiquidInfo {
    bool water = false;
    bool lava = false;
    bool bubble = false;
    bool dragDown = false;
    bool source = false;
    bool falling = false;
    int decay = 0;
    float height = 0.0f;
};

struct LiquidChange {
    Vector3i position;
    BlockState state;
};

class Level;

class LiquidPhysicsSystem {
public:
    explicit LiquidPhysicsSystem(Level &level) : mLevel(level) {}

    void moveStateFrom(LiquidPhysicsSystem &&other);

    void onChunkLoaded(Chunk &chunk);
    void onBlockChanged(int32_t x, int32_t y, int32_t z);

    LiquidInfo getLiquidInfo(int32_t x, int32_t y, int32_t z);
    Vector3f getFlowVector(const Vector3i &position);

    void schedule(const Vector3i &position, int64_t delay = 1);
    void tick();
    std::vector<LiquidChange> consumeChanges();

private:
    struct Position {
        int32_t x;
        int32_t y;
        int32_t z;

        bool operator==(const Position &other) const {
            return x == other.x && y == other.y && z == other.z;
        }
    };

    struct PositionHash {
        size_t operator()(const Position &position) const {
            size_t result = std::hash<int32_t>()(position.x);
            result ^= std::hash<int32_t>()(position.y) + (result << 6) + (result >> 2);
            result ^= std::hash<int32_t>()(position.z) + (result << 6) + (result >> 2);
            return result;
        }
    };

    bool isFluidState(const BlockState &state) const;
    bool isSameFluid(const BlockState &left, const BlockState &right) const;
    bool isFlowable(const BlockState &state) const;
    void scheduleNeighbors(int32_t x, int32_t y, int32_t z);
    void scheduleLoaded(Chunk &chunk);
    void process(const Vector3i &position);
    void processBubbleColumn(const Vector3i &position);
    void setFluidState(const Vector3i &position, const BlockState &state);
    void harden(const Vector3i &position);
    BlockState makeState(bool lava, int decay, bool falling) const;

    Level &mLevel;
    int64_t mTick = 0;
    std::unordered_map<Position, int64_t, PositionHash> mSchedule;
    std::vector<LiquidChange> mChanges;
};
