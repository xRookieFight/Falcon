#include "Block/Systems/LiquidPhysicsSystem.h"

#include "Block/BlockData.h"
#include "Block/Blocks/LiquidView.h"
#include "Level/Level.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <vector>

void LiquidPhysicsSystem::moveStateFrom(LiquidPhysicsSystem &&other) {
    if (this == &other)
        return;
    mTick = other.mTick;
    mSchedule = std::move(other.mSchedule);
    mBuckets = std::move(other.mBuckets);
    mParked = std::move(other.mParked);
    mChanges = std::move(other.mChanges);
}

const BlockState &LiquidPhysicsSystem::_stateAt(int32_t x, int32_t y, int32_t z) {
    static const BlockState bedrock("minecraft:bedrock");

    const BlockState *state = mLevel.peekBlockPtr(x, y, z);
    return state == nullptr ? bedrock : *state;
}

bool LiquidPhysicsSystem::_isLoaded(int32_t x, int32_t z) const {
    return mLevel.isChunkResident(x >> 4, z >> 4);
}

bool LiquidPhysicsSystem::_isActive(int32_t x, int32_t z) const {
    return mLevel.isColumnActive(x >> 4, z >> 4);
}

void LiquidPhysicsSystem::activateColumn(int32_t chunkX, int32_t chunkZ) {
    const int64_t column = ((int64_t) chunkX << 32) | (uint32_t) chunkZ;

    auto it = mParked.find(column);
    if (it == mParked.end())
        return;

    std::vector<Position> parked;
    parked.swap(it->second);
    mParked.erase(it);

    for (const Position &position: parked)
        schedule(Vector3i(position.x, position.y, position.z), 1);
}

void LiquidPhysicsSystem::onChunkLoaded(LevelChunk &chunk) {
    scheduleLoaded(chunk);
}

void LiquidPhysicsSystem::onBlockChanged(int32_t x, int32_t y, int32_t z) {
    scheduleNeighbors(x, y, z);
}

LiquidInfo LiquidPhysicsSystem::getLiquidInfo(int32_t x, int32_t y, int32_t z) {
    LiquidInfo result;
    if (y < LevelChunk::MIN_Y || y > LevelChunk::MAX_Y)
        return result;

    const LiquidView block(_stateAt(x, y, z));
    result.water = block.isWater();
    result.lava = block.isLava();
    result.bubble = block.isBubbleColumn();
    result.dragDown = result.bubble && block.isDragDown();
    result.source = block.isSource();
    result.falling = block.isFalling();
    result.decay = block.getDecay();
    result.height = block.isLiquid() || result.bubble ? block.getFluidHeightPercent() : 0.0f;
    return result;
}

Vector3f LiquidPhysicsSystem::getFlowVector(const Vector3i &position) {
    const LiquidView liquid(_stateAt(position.x, position.y, position.z));
    if (!liquid.isLiquid())
        return Vector3f();

    const bool lava = liquid.isLava();
    const int decay = liquid.isSource() || liquid.isFalling() ? 0 : liquid.getDecay();
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    static const int offsets[4][3] = {{0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}};

    for (const auto &offset: offsets) {
        const int32_t nx = position.x + offset[0];
        const int32_t ny = position.y;
        const int32_t nz = position.z + offset[2];
        const BlockState &side = _stateAt(nx, ny, nz);
        const LiquidView sideLiquid(side);
        if (sideLiquid.isLiquid() && sideLiquid.isLava() == lava) {
            const int sideDecay = sideLiquid.isSource() || sideLiquid.isFalling() ? 0 : sideLiquid.getDecay();
            const int realDecay = sideDecay - decay;
            x += (float) offset[0] * (float) realDecay;
            z += (float) offset[2] * (float) realDecay;
        } else if (isFlowable(side)) {
            const BlockState &below = _stateAt(nx, ny - 1, nz);
            const LiquidView belowLiquid(below);
            if (belowLiquid.isLiquid() && belowLiquid.isLava() == lava) {
                const int belowDecay = belowLiquid.isSource() || belowLiquid.isFalling() ? 0 : belowLiquid.getDecay();
                const int realDecay = belowDecay - (decay - 8);
                x += (float) offset[0] * (float) realDecay;
                z += (float) offset[2] * (float) realDecay;
            }
        }
    }

    if (liquid.isFalling()) {
        for (const auto &offset: offsets) {
            const BlockState side = _stateAt(position.x + offset[0], position.y,
                                                         position.z + offset[2]);
            const BlockState above = _stateAt(position.x + offset[0], position.y + 1,
                                                          position.z + offset[2]);
            if (!isFlowable(side) || !isFlowable(above)) {
                const float length = std::sqrt(x * x + y * y + z * z);
                if (length > 0.0f) {
                    x /= length;
                    z /= length;
                }
                y -= 6.0f;
                break;
            }
        }
    }

    const float length = std::sqrt(x * x + y * y + z * z);
    if (length <= 0.0f)
        return Vector3f();
    return Vector3f(x / length, y / length, z / length);
}

void LiquidPhysicsSystem::schedule(const Vector3i &position, int64_t delay) {
    if (position.y < LevelChunk::MIN_Y || position.y > LevelChunk::MAX_Y)
        return;

    const Position key{position.x, position.y, position.z};
    const int64_t due = mTick + std::max<int64_t>(1, delay);

    auto it = mSchedule.find(key);
    if (it != mSchedule.end() && due >= it->second)
        return;

    mSchedule[key] = due;
    mBuckets[due].push_back(key);
}

std::vector<LiquidChange> LiquidPhysicsSystem::consumeChanges() {
    std::vector<LiquidChange> result;
    result.swap(mChanges);
    return result;
}

bool LiquidPhysicsSystem::isFluidState(const BlockState &state) const {
    return state.mName == "minecraft:water" || state.mName == "minecraft:flowing_water"
           || state.mName == "minecraft:lava" || state.mName == "minecraft:flowing_lava"
           || state.mName == "minecraft:bubble_column";
}

bool LiquidPhysicsSystem::isSameFluid(const BlockState &left, const BlockState &right) const {
    const bool leftWater = left.mName == "minecraft:water" || left.mName == "minecraft:flowing_water"
                           || left.mName == "minecraft:bubble_column";
    const bool rightWater = right.mName == "minecraft:water" || right.mName == "minecraft:flowing_water"
                            || right.mName == "minecraft:bubble_column";
    const bool leftLava = left.mName == "minecraft:lava" || left.mName == "minecraft:flowing_lava";
    const bool rightLava = right.mName == "minecraft:lava" || right.mName == "minecraft:flowing_lava";
    return (leftWater && rightWater) || (leftLava && rightLava);
}

bool LiquidPhysicsSystem::isFlowable(const BlockState &state) const {
    if (state.mName == "minecraft:air" || isFluidState(state))
        return true;
    const BlockData *data = BlockDataTable::find(state.mName.c_str());
    return data != nullptr && !data->mSolid;
}

int64_t LiquidPhysicsSystem::getTickRate(const BlockState &state) const {
    const LiquidView liquid(state);
    if (liquid.isLiquid() || liquid.isBubbleColumn())
        return liquid.getTickRate();
    return 1;
}

void LiquidPhysicsSystem::scheduleNeighbors(int32_t x, int32_t y, int32_t z) {
    static const int offsets[7][3] = {
            {0, 0, 0}, {0, -1, 0}, {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}
    };

    for (const auto &offset: offsets) {
        const Vector3i position(x + offset[0], y + offset[1], z + offset[2]);
        const BlockState state = _stateAt(position.x, position.y, position.z);
        const LiquidView liquid(state);
        if (liquid.isLiquid() || liquid.isBubbleColumn())
            schedule(position, getTickRate(state));
    }
}

void LiquidPhysicsSystem::scheduleLoaded(LevelChunk &chunk) {
    chunk.forEachBlock([this](int32_t x, int32_t y, int32_t z, const BlockState &state) {
        if (isFluidState(state))
            schedule(Vector3i(x, y, z), getTickRate(state));
    });
}

BlockState LiquidPhysicsSystem::makeState(bool lava, int decay, bool falling) const {
    Tag states = Tag::ofCompound();
    states.putInt("liquid_depth", std::clamp(decay, 0, 7));
    const bool source = !falling && decay == 0;
    const std::string name = lava
                             ? (source ? "minecraft:lava" : "minecraft:flowing_lava")
                             : (source ? "minecraft:water" : "minecraft:flowing_water");
    return BlockState(name, states);
}

void LiquidPhysicsSystem::setFluidState(const Vector3i &position, const BlockState &state) {
    if (!_isLoaded(position.x, position.z))
        return;

    const BlockState current = _stateAt(position.x, position.y, position.z);
    if (current == state)
        return;
    mLevel.setBlockState(position.x, position.y, position.z, state);
    mChanges.push_back(LiquidChange{position, state});
}

void LiquidPhysicsSystem::harden(const Vector3i &position) {
    const BlockState current = _stateAt(position.x, position.y, position.z);
    const LiquidView liquid(current);
    if (!liquid.isLava() || liquid.isFalling())
        return;

    static const int offsets[6][3] = {
            {0, -1, 0}, {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}
    };
    for (const auto &offset: offsets) {
        const Vector3i neighbor(position.x + offset[0], position.y + offset[1], position.z + offset[2]);
        const BlockState otherState = _stateAt(neighbor.x, neighbor.y, neighbor.z);
        const LiquidView other(otherState);
        if (!other.isLiquid() || other.isWater() == liquid.isWater())
            continue;

        const std::string result = liquid.isSource() ? "minecraft:obsidian"
                              : liquid.getDecay() <= 4 ? "minecraft:cobblestone" : "minecraft:stone";
        setFluidState(position, BlockState(result));
        return;
    }
}

bool LiquidPhysicsSystem::resolveFluidCollision(const Vector3i &target, const BlockState &sourceState,
                                                bool downward) {
    if (target.y < LevelChunk::MIN_Y || target.y > LevelChunk::MAX_Y)
        return false;

    const BlockState targetState = _stateAt(target.x, target.y, target.z);
    if (!isFluidState(targetState) || !isFluidState(sourceState))
        return false;
    if (isSameFluid(targetState, sourceState))
        return false;

    const LiquidView targetLiquid(targetState);
    std::string result;

    if (targetLiquid.isLava()) {
        result = targetLiquid.isSource() ? "minecraft:obsidian"
               : targetLiquid.getDecay() <= 4 ? "minecraft:cobblestone"
               : "minecraft:stone";
    } else {
        result = downward ? "minecraft:stone" : "minecraft:cobblestone";
    }

    setFluidState(target, BlockState(result));
    return true;
}

void LiquidPhysicsSystem::processBubbleColumn(const Vector3i &position) {
    const BlockState state = _stateAt(position.x, position.y, position.z);
    const LiquidView column(state);
    if (!column.isBubbleColumn())
        return;

    const BlockState below = _stateAt(position.x, position.y - 1, position.z);
    const bool supported = below.mName == "minecraft:magma_block" ? column.isDragDown()
                         : below.mName == "minecraft:soul_sand" ? !column.isDragDown()
                         : LiquidView(below).isBubbleColumn()
                           && LiquidView(below).isDragDown() == column.isDragDown();
    if (!supported) {
        setFluidState(position, makeState(false, 0, false));
        return;
    }

    const BlockState above = _stateAt(position.x, position.y + 1, position.z);
    const LiquidView aboveLiquid(above);
    if (aboveLiquid.isSource() && aboveLiquid.isWater()) {
        Tag states = Tag::ofCompound();
        states.putByte("drag_down", column.isDragDown() ? 1 : 0);
        setFluidState(Vector3i(position.x, position.y + 1, position.z),
                      BlockState("minecraft:bubble_column", states));
    }
    schedule(position, 5);
}

void LiquidPhysicsSystem::process(const Vector3i &position) {
    if (!_isLoaded(position.x, position.z))
        return;

    const BlockState state = _stateAt(position.x, position.y, position.z);
    const LiquidView liquid(state);
    if (liquid.isBubbleColumn()) {
        processBubbleColumn(position);
        return;
    }
    if (!liquid.isLiquid())
        return;

    harden(position);
    const BlockState currentState = _stateAt(position.x, position.y, position.z);
    const LiquidView current(currentState);
    if (!current.isLiquid())
        return;

    if (current.isWater() && current.isSource()) {
        const BlockState below = _stateAt(position.x, position.y - 1, position.z);
        if (below.mName == "minecraft:magma_block" || below.mName == "minecraft:soul_sand") {
            Tag states = Tag::ofCompound();
            states.putByte("drag_down", below.mName == "minecraft:magma_block" ? 1 : 0);
            setFluidState(position, BlockState("minecraft:bubble_column", states));
            return;
        }
    }

    const bool source = current.isSource();
    const bool lava = current.isLava();
    const int step = lava ? LiquidView(current).getFlowDecayPerBlock()
                          : LiquidView(current).getFlowDecayPerBlock();
    const Vector3i belowPosition(position.x, position.y - 1, position.z);
    resolveFluidCollision(belowPosition, currentState, true);

    const BlockState below = _stateAt(belowPosition.x, belowPosition.y, belowPosition.z);
    const bool belowFlowable = isFlowable(below) && !isSameFluid(below, currentState);
    if (belowFlowable) {
        setFluidState(belowPosition, makeState(lava, 0, true));
        if (!source)
            schedule(position, lava ? LiquidView(current).getTickRate() : LiquidView(current).getTickRate());
        return;
    }

    const int baseDecay = source || current.isFalling() ? 0 : current.getDecay();
    const int nextDecay = baseDecay + step;
    if (nextDecay <= 7) {
        static const int offsets[4][3] = {{-1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}};
        for (const auto &offset: offsets) {
            const Vector3i side(position.x + offset[0], position.y, position.z + offset[2]);
            if (resolveFluidCollision(side, currentState, false))
                continue;

            const BlockState sideState = _stateAt(side.x, side.y, side.z);
            if (!isFlowable(sideState) || (isSameFluid(sideState, currentState) && LiquidView(sideState).isSource()))
                continue;

            if (isSameFluid(sideState, currentState)) {
                const LiquidView sideLiquid(sideState);
                const int sideDecay = sideLiquid.isFalling() ? 0 : sideLiquid.getDecay();
                if (sideDecay <= nextDecay)
                    continue;
            }
            setFluidState(side, makeState(lava, nextDecay, false));
        }
    }

    if (!source) {
        if (current.isWater()) {
            int sources = 0;
            static const int sourceOffsets[4][3] = {{-1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}};
            for (const auto &offset: sourceOffsets) {
                const LiquidView side(_stateAt(position.x + offset[0], position.y,
                                                            position.z + offset[2]));
                if (side.isWater() && side.isSource())
                    ++sources;
            }
            const LiquidView belowLiquid(below);
            const BlockData *belowData = BlockDataTable::find(below.mName.c_str());
            const int minSources = LiquidView(current).getMinAdjacentSourcesToFormSource();
            if (sources >= minSources && ((belowData != nullptr && belowData->mSolid)
                                 || (belowLiquid.isWater() && belowLiquid.isSource()))) {
                setFluidState(position, makeState(false, 0, false));
                return;
            }
        }

        int smallest = std::numeric_limits<int>::max();
        static const int offsets[4][3] = {{-1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}};
        for (const auto &offset: offsets) {
            const BlockState side = _stateAt(position.x + offset[0], position.y,
                                                         position.z + offset[2]);
            if (!isSameFluid(side, currentState))
                continue;
            const LiquidView sideLiquid(side);
            const int sideDecay = sideLiquid.isSource() || sideLiquid.isFalling() ? 0 : sideLiquid.getDecay();
            smallest = std::min(smallest, sideDecay);
        }

        const BlockState above = _stateAt(position.x, position.y + 1, position.z);
        if (isSameFluid(above, currentState)) {
            const LiquidView aboveLiquid(above);
            if (aboveLiquid.isSource() || aboveLiquid.isFalling())
                smallest = 0;
        }

        if (smallest == std::numeric_limits<int>::max() || smallest + step > 7) {
            setFluidState(position, BlockState("minecraft:air"));
            return;
        }

        const int newDecay = smallest + step;
        if (newDecay != current.getDecay() || current.isFalling())
            setFluidState(position, makeState(lava, newDecay, false));
    }
}

void LiquidPhysicsSystem::_deferRemaining(const std::vector<Position> &positions, size_t from, int64_t bucketTick) {
    if (from >= positions.size())
        return;

    std::vector<Position> &target = mBuckets[bucketTick];
    target.insert(target.end(), positions.begin() + (long) from, positions.end());
    mLastDeferred += positions.size() - from;
}

void LiquidPhysicsSystem::tick() {
    ++mTick;
    mLastProcessed = 0;
    mLastDeferred = 0;

    if (mBuckets.empty())
        return;

    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    size_t sinceCheck = 0;
    bool exhausted = false;

    while (!mBuckets.empty() && !exhausted) {
        auto bucket = mBuckets.begin();
        if (bucket->first > mTick)
            break;

        std::vector<Position> positions;
        positions.swap(bucket->second);
        const int64_t bucketTick = bucket->first;
        mBuckets.erase(bucket);

        for (size_t i = 0; i < positions.size(); ++i) {
            const Position &key = positions[i];

            auto scheduled = mSchedule.find(key);
            if (scheduled != mSchedule.end() && scheduled->second == bucketTick) {
                if (!_isActive(key.x, key.z)) {
                    mSchedule.erase(scheduled);
                    const int64_t column = ((int64_t) (key.x >> 4) << 32) | (uint32_t) (key.z >> 4);
                    mParked[column].push_back(key);
                } else {
                    mSchedule.erase(scheduled);
                    process(Vector3i(key.x, key.y, key.z));
                    mLastProcessed++;

                    if (mLastProcessed >= MAX_UPDATES_PER_TICK) {
                        _deferRemaining(positions, i + 1, bucketTick);
                        exhausted = true;
                        break;
                    }
                }
            }

            if (++sinceCheck < BUDGET_CHECK_INTERVAL)
                continue;

            sinceCheck = 0;

            const double elapsed = std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - start).count();

            if (elapsed < mTimeBudgetMs)
                continue;

            _deferRemaining(positions, i + 1, bucketTick);
            exhausted = true;
            break;
        }
    }
}
