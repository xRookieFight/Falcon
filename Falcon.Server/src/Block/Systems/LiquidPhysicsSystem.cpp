#include "Block/Systems/LiquidPhysicsSystem.h"

#include "Block/BlockData.h"
#include "Block/Blocks/BubbleColumnBlock.h"
#include "Block/Blocks/LavaBlock.h"
#include "Block/Blocks/LiquidBlock.h"
#include "Block/Blocks/WaterBlock.h"
#include "Level/Level.h"

#include <algorithm>
#include <cmath>
#include <limits>

void LiquidPhysicsSystem::moveStateFrom(LiquidPhysicsSystem &&other) {
    if (this == &other)
        return;
    mTick = other.mTick;
    mSchedule = std::move(other.mSchedule);
    mChanges = std::move(other.mChanges);
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

    const LiquidBlock block(mLevel.getBlockState(x, y, z));
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
    const LiquidBlock liquid(mLevel.getBlockState(position.x, position.y, position.z));
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
        const BlockState side = mLevel.getBlockState(nx, ny, nz);
        const LiquidBlock sideLiquid(side);
        if (sideLiquid.isLiquid() && sideLiquid.isLava() == lava) {
            const int sideDecay = sideLiquid.isSource() || sideLiquid.isFalling() ? 0 : sideLiquid.getDecay();
            const int realDecay = sideDecay - decay;
            x += (float) offset[0] * (float) realDecay;
            z += (float) offset[2] * (float) realDecay;
        } else if (isFlowable(side)) {
            const BlockState below = mLevel.getBlockState(nx, ny - 1, nz);
            const LiquidBlock belowLiquid(below);
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
            const BlockState side = mLevel.getBlockState(position.x + offset[0], position.y,
                                                         position.z + offset[2]);
            const BlockState above = mLevel.getBlockState(position.x + offset[0], position.y + 1,
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
    if (it == mSchedule.end() || due < it->second)
        mSchedule[key] = due;
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

void LiquidPhysicsSystem::scheduleNeighbors(int32_t x, int32_t y, int32_t z) {
    static const int offsets[7][3] = {
            {0, 0, 0}, {0, -1, 0}, {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}
    };

    for (const auto &offset: offsets)
        schedule(Vector3i(x + offset[0], y + offset[1], z + offset[2]));
}

void LiquidPhysicsSystem::scheduleLoaded(LevelChunk &chunk) {
    chunk.forEachBlock([this](int32_t x, int32_t y, int32_t z, const BlockState &state) {
        if (isFluidState(state))
            schedule(Vector3i(x, y, z), 1);
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
    const BlockState current = mLevel.getBlockState(position.x, position.y, position.z);
    if (current == state)
        return;
    mLevel.setBlockState(position.x, position.y, position.z, state);
    mChanges.push_back(LiquidChange{position, state});
}

void LiquidPhysicsSystem::harden(const Vector3i &position) {
    const BlockState current = mLevel.getBlockState(position.x, position.y, position.z);
    const LiquidBlock liquid(current);
    if (!liquid.isLava() || liquid.isFalling())
        return;

    static const int offsets[6][3] = {
            {0, -1, 0}, {0, 1, 0}, {-1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}
    };
    for (const auto &offset: offsets) {
        const Vector3i neighbor(position.x + offset[0], position.y + offset[1], position.z + offset[2]);
        const BlockState otherState = mLevel.getBlockState(neighbor.x, neighbor.y, neighbor.z);
        const LiquidBlock other(otherState);
        if (!other.isLiquid() || other.isWater() == liquid.isWater())
            continue;

        const std::string result = liquid.isSource() ? "minecraft:obsidian"
                              : liquid.getDecay() <= 4 ? "minecraft:cobblestone" : "minecraft:stone";
        setFluidState(position, BlockState(result));
        return;
    }
}

void LiquidPhysicsSystem::processBubbleColumn(const Vector3i &position) {
    const BlockState state = mLevel.getBlockState(position.x, position.y, position.z);
    const BubbleColumnBlock column(state);
    if (!column.isBubbleColumn())
        return;

    const BlockState below = mLevel.getBlockState(position.x, position.y - 1, position.z);
    const bool supported = below.mName == "minecraft:magma_block" ? column.isDragDown()
                         : below.mName == "minecraft:soul_sand" ? !column.isDragDown()
                         : BubbleColumnBlock(below).isBubbleColumn()
                           && BubbleColumnBlock(below).isDragDown() == column.isDragDown();
    if (!supported) {
        setFluidState(position, makeState(false, 0, false));
        return;
    }

    const BlockState above = mLevel.getBlockState(position.x, position.y + 1, position.z);
    const LiquidBlock aboveLiquid(above);
    if (aboveLiquid.isSource() && aboveLiquid.isWater()) {
        Tag states = Tag::ofCompound();
        states.putByte("drag_down", column.isDragDown() ? 1 : 0);
        setFluidState(Vector3i(position.x, position.y + 1, position.z),
                      BlockState("minecraft:bubble_column", states));
    }
    schedule(position, 5);
}

void LiquidPhysicsSystem::process(const Vector3i &position) {
    const BlockState state = mLevel.getBlockState(position.x, position.y, position.z);
    const LiquidBlock liquid(state);
    if (liquid.isBubbleColumn()) {
        processBubbleColumn(position);
        return;
    }
    if (!liquid.isLiquid())
        return;

    harden(position);
    const BlockState currentState = mLevel.getBlockState(position.x, position.y, position.z);
    const LiquidBlock current(currentState);
    if (!current.isLiquid())
        return;

    if (current.isWater() && current.isSource()) {
        const BlockState below = mLevel.getBlockState(position.x, position.y - 1, position.z);
        if (below.mName == "minecraft:magma_block" || below.mName == "minecraft:soul_sand") {
            Tag states = Tag::ofCompound();
            states.putByte("drag_down", below.mName == "minecraft:magma_block" ? 1 : 0);
            setFluidState(position, BlockState("minecraft:bubble_column", states));
            return;
        }
    }

    const bool source = current.isSource();
    const bool lava = current.isLava();
    const int step = lava ? LavaBlock(current).getFlowDecayPerBlock()
                          : WaterBlock(current).getFlowDecayPerBlock();
    const BlockState below = mLevel.getBlockState(position.x, position.y - 1, position.z);
    const bool belowFlowable = isFlowable(below) && !isSameFluid(below, currentState);
    if (belowFlowable) {
        setFluidState(Vector3i(position.x, position.y - 1, position.z), makeState(lava, 0, true));
        if (!source)
            schedule(position, lava ? LavaBlock(current).getTickRate() : WaterBlock(current).getTickRate());
        return;
    }

    const int baseDecay = source || current.isFalling() ? 0 : current.getDecay();
    const int nextDecay = baseDecay + step;
    if (nextDecay <= 7) {
        static const int offsets[4][3] = {{-1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}};
        for (const auto &offset: offsets) {
            const Vector3i side(position.x + offset[0], position.y, position.z + offset[2]);
            const BlockState sideState = mLevel.getBlockState(side.x, side.y, side.z);
            if (!isFlowable(sideState) || (isSameFluid(sideState, currentState) && LiquidBlock(sideState).isSource()))
                continue;

            if (isSameFluid(sideState, currentState)) {
                const LiquidBlock sideLiquid(sideState);
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
                const LiquidBlock side(mLevel.getBlockState(position.x + offset[0], position.y,
                                                            position.z + offset[2]));
                if (side.isWater() && side.isSource())
                    ++sources;
            }
            const LiquidBlock belowLiquid(below);
            const BlockData *belowData = BlockDataTable::find(below.mName.c_str());
            const int minSources = WaterBlock(current).getMinAdjacentSourcesToFormSource();
            if (sources >= minSources && ((belowData != nullptr && belowData->mSolid)
                                 || (belowLiquid.isWater() && belowLiquid.isSource()))) {
                setFluidState(position, makeState(false, 0, false));
                return;
            }
        }

        int smallest = std::numeric_limits<int>::max();
        static const int offsets[4][3] = {{-1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}};
        for (const auto &offset: offsets) {
            const BlockState side = mLevel.getBlockState(position.x + offset[0], position.y,
                                                         position.z + offset[2]);
            if (!isSameFluid(side, currentState))
                continue;
            const LiquidBlock sideLiquid(side);
            const int sideDecay = sideLiquid.isSource() || sideLiquid.isFalling() ? 0 : sideLiquid.getDecay();
            smallest = std::min(smallest, sideDecay);
        }

        const BlockState above = mLevel.getBlockState(position.x, position.y + 1, position.z);
        if (isSameFluid(above, currentState)) {
            const LiquidBlock aboveLiquid(above);
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

void LiquidPhysicsSystem::tick() {
    ++mTick;
    size_t processed = 0;
    for (auto it = mSchedule.begin(); it != mSchedule.end() && processed < 4096;) {
        if (it->second > mTick) {
            ++it;
            continue;
        }
        const Vector3i position(it->first.x, it->first.y, it->first.z);
        it = mSchedule.erase(it);
        process(position);
        ++processed;
    }
}
