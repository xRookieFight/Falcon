#pragma once

#include "Core/Math/Vector3f.h"
#include "Core/Math/Vector3i.h"
#include "Level/LevelChunk.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <unordered_map>
#include <unordered_set>
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

    void onChunkLoaded(LevelChunk &chunk);
    void onBlockChanged(int32_t x, int32_t y, int32_t z);

    LiquidInfo getLiquidInfo(int32_t x, int32_t y, int32_t z);
    Vector3f getFlowVector(const Vector3i &position);

    void schedule(const Vector3i &position, int64_t delay = 1);
    void processImmediately(const Vector3i &position);

    void onScheduledUpdate(const Vector3i &position);

    static bool needsInitialTick(const LevelChunk &chunk, int32_t localX, int32_t y, int32_t localZ);

    std::vector<LiquidChange> consumeChanges();

private:
    static bool _canFlowInto(const BlockState &source, const BlockState &target);

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
    int64_t getTickRate(const BlockState &state) const;
    const BlockState &_stateAt(int32_t x, int32_t y, int32_t z);

    bool _isLoaded(int32_t x, int32_t z) const;


    void _enqueueImmediate(const Position &position);

    bool _canBeFlowedInto(const BlockState &state) const;
    int _calculateFlowCost(int32_t x, int32_t y, int32_t z, int accumulatedCost, int maxCost,
                           int originOpposite, int lastOpposite,
                           std::unordered_map<Position, int8_t, PositionHash> &visited);
    void _getOptimalFlowDirections(int32_t x, int32_t y, int32_t z, int decayPerBlock, bool out[4]);

    void scheduleNeighbors(int32_t x, int32_t y, int32_t z);
    void scheduleLoaded(LevelChunk &chunk);
    void process(const Vector3i &position);
    void processBubbleColumn(const Vector3i &position);
    void setFluidState(const Vector3i &position, const BlockState &state);
    void harden(const Vector3i &position);
    bool resolveFluidCollision(const Vector3i &target, const BlockState &sourceState, bool downward);
    BlockState makeState(bool lava, int decay, bool falling) const;

    static constexpr int8_t FLOW_BLOCKED = -1;
    static constexpr int8_t FLOW_CAN_FLOW = 0;
    static constexpr int8_t FLOW_CAN_FLOW_DOWN = 1;

    Level &mLevel;
    std::vector<LiquidChange> mChanges;
    bool mImmediatePropagation = false;
    std::vector<Position> mImmediateQueue;
    std::unordered_set<Position, PositionHash> mImmediatePending;
};
