#include "Block/Systems/PistonSystem.h"

#include "Block/Actor/PistonArmBlockActor.h"
#include "Block/Block.h"
#include "Block/BlockActorStore.h"
#include "Block/BlockData.h"
#include "Block/Blocks/VanillaBlocks.h"
#include "Block/Systems/RedstoneSystem.h"
#include "Level/Level.h"
#include "Network/Handler/BlockActionHandler.h"
#include "Network/Handler/ServerNetworkHandler.h"
#include "Protocol/Packets/BlockActorDataPacket.h"
#include "Protocol/Packets/LevelSoundEventPacket.h"

#include <algorithm>
#include <unordered_set>

namespace {
    const char *PISTON = "minecraft:piston";
    const char *STICKY_PISTON = "minecraft:sticky_piston";
    const char *PISTON_ARM_COLLISION = "minecraft:piston_arm_collision";
    const char *STICKY_PISTON_ARM_COLLISION = "minecraft:sticky_piston_arm_collision";
    const char *AIR = "minecraft:air";

    const std::unordered_set<std::string> UNPUSHABLE = {
            "minecraft:bedrock", "minecraft:obsidian", "minecraft:crying_obsidian",
            "minecraft:glowingobsidian", "minecraft:beacon", "minecraft:barrier",
            "minecraft:allow", "minecraft:deny", "minecraft:border_block",
            "minecraft:command_block", "minecraft:chain_command_block", "minecraft:repeating_command_block",
            "minecraft:ender_chest", "minecraft:end_gateway", "minecraft:end_portal",
            "minecraft:end_portal_frame", "minecraft:frog_spawn", "minecraft:invisible_bedrock",
            "minecraft:jigsaw", "minecraft:lodestone", "minecraft:mob_spawner", "minecraft:moving_block",
            "minecraft:pointed_dripstone", "minecraft:portal", "minecraft:respawn_anchor",
            "minecraft:sculk_catalyst", "minecraft:sculk_shrieker", "minecraft:structure_block",
            "minecraft:structure_void", "minecraft:trial_spawner", "minecraft:vault",
            "minecraft:creaking_heart"
    };

    const std::unordered_set<std::string> UNPULLABLE_EXTRA = {
            "minecraft:campfire", "minecraft:soul_campfire", "minecraft:heavy_core"
    };

    const std::unordered_set<std::string> BREAKS_WHEN_MOVED = {
            "minecraft:bamboo", "minecraft:bed", "minecraft:budding_amethyst", "minecraft:cactus",
            "minecraft:cake", "minecraft:campfire", "minecraft:soul_campfire", "minecraft:candle_cake",
            "minecraft:chorus_flower", "minecraft:chorus_plant", "minecraft:cocoa", "minecraft:dragon_egg",
            "minecraft:ladder", "minecraft:lever", "minecraft:glow_lichen", "minecraft:melon_block",
            "minecraft:pumpkin", "minecraft:snow_layer", "minecraft:standing_banner",
            "minecraft:wall_banner", "minecraft:undyed_shulker_box", "minecraft:vine",
            "minecraft:frame", "minecraft:glow_frame"
    };

    const std::unordered_set<std::string> NEVER_BREAKS = {
            "minecraft:rail", "minecraft:golden_rail", "minecraft:detector_rail", "minecraft:activator_rail",
            "minecraft:sculk_sensor", "minecraft:calibrated_sculk_sensor", "minecraft:sculk_shrieker",
            "minecraft:heavy_core"
    };

    struct PendingMove {
        Vector3i mPiston;
        std::vector<BlockState> mMoved;
        int mDirection;
    };

    std::vector<PendingMove> gPendingMoves;

    void broadcastArmData(ServerNetworkHandler &owner, const PistonArmBlockActor &arm) {
        const Vector3i &position = arm.getPosition();

        BlockActorDataPacket packet;
        packet.mBlockPosition = position;
        packet.mData = arm.getSpawnCompound();

        const Vector3f center((float) position.x + 0.5f, (float) position.y + 0.5f, (float) position.z + 0.5f);
        BlockActionHandler::broadcastToViewers(owner, center, packet);
    }

    bool contains(const std::unordered_set<std::string> &set, const std::string &value) {
        return set.find(value) != set.end();
    }

    bool isDoor(const std::string &identifier) {
        return identifier.find("_door") != std::string::npos;
    }

    bool isLeaves(const std::string &identifier) {
        return identifier.find("leaves") != std::string::npos;
    }

    bool isSign(const std::string &identifier) {
        return identifier.find("_sign") != std::string::npos || identifier == "minecraft:standing_sign"
               || identifier == "minecraft:wall_sign";
    }

    bool isHead(const std::string &identifier) {
        return identifier.find("_head") != std::string::npos || identifier.find("skull") != std::string::npos;
    }

    bool isLiquid(const std::string &identifier) {
        return identifier == "minecraft:water" || identifier == "minecraft:flowing_water"
               || identifier == "minecraft:lava" || identifier == "minecraft:flowing_lava";
    }

    bool isGlazedTerracotta(const std::string &identifier) {
        return identifier.find("glazed_terracotta") != std::string::npos;
    }

    bool isFlowable(const BlockState &state) {
        const Block *block = VanillaBlocks::fromIdentifier(state.mName);
        if (block == nullptr)
            return false;

        const BlockData *data = block->getData();
        return data != nullptr && !data->mSolid;
    }

    BlockState stateAt(ServerNetworkHandler &owner, const Vector3i &position) {
        return owner.getLevel().getBlockState(position.x, position.y, position.z);
    }

    bool isAir(const BlockState &state) {
        return state.mName == AIR;
    }

    Vector3i relative(const Vector3i &position, int face, int distance = 1) {
        const Vector3i offset = RedstoneFace::offset(face);
        return Vector3i(position.x + offset.x * distance, position.y + offset.y * distance,
                        position.z + offset.z * distance);
    }

    int faceAxis(int face) {
        if (face == RedstoneFace::DOWN || face == RedstoneFace::UP)
            return 1;
        if (face == RedstoneFace::NORTH || face == RedstoneFace::SOUTH)
            return 2;
        return 0;
    }
}

bool PistonSystem::isPiston(const std::string &identifier) {
    return identifier == PISTON || identifier == STICKY_PISTON;
}

bool PistonSystem::isSticky(const std::string &identifier) {
    return identifier == STICKY_PISTON || identifier == STICKY_PISTON_ARM_COLLISION;
}

bool PistonSystem::isArmCollision(const std::string &identifier) {
    return identifier == PISTON_ARM_COLLISION || identifier == STICKY_PISTON_ARM_COLLISION;
}

int PistonSystem::getPistonFace(const BlockState &state) {
    const int index = getStoredFacing(state);
    if (index == RedstoneFace::DOWN || index == RedstoneFace::UP)
        return index;

    return RedstoneFace::opposite(index);
}

int PistonSystem::getStoredFacing(const BlockState &state) {
    const Tag *facing = state.mStates.get("facing_direction");
    if (facing == nullptr)
        return RedstoneFace::UP;

    const int index = facing->asInt();
    if (index < 0 || index >= RedstoneFace::COUNT)
        return RedstoneFace::UP;

    return index;
}

bool PistonSystem::canBePushed(const BlockState &state) {
    if (isAir(state))
        return true;
    if (contains(UNPUSHABLE, state.mName) || isArmCollision(state.mName))
        return false;

    return true;
}

bool PistonSystem::canBePulled(const BlockState &state) {
    if (!canBePushed(state))
        return false;

    return !contains(UNPULLABLE_EXTRA, state.mName) && !isGlazedTerracotta(state.mName);
}

bool PistonSystem::breaksWhenMoved(const BlockState &state) {
    if (isAir(state))
        return false;
    if (contains(NEVER_BREAKS, state.mName))
        return false;
    if (contains(BREAKS_WHEN_MOVED, state.mName))
        return true;
    if (isDoor(state.mName) || isLeaves(state.mName) || isSign(state.mName) || isHead(state.mName)
        || isLiquid(state.mName))
        return true;

    return isFlowable(state);
}

bool PistonSystem::sticksToPiston(const BlockState &state) {
    if (contains(NEVER_BREAKS, state.mName))
        return true;
    if (contains(BREAKS_WHEN_MOVED, state.mName))
        return false;
    if (isDoor(state.mName) || isLeaves(state.mName) || isSign(state.mName) || isHead(state.mName)
        || isLiquid(state.mName) || isGlazedTerracotta(state.mName))
        return false;

    return !isFlowable(state);
}

bool PistonSystem::canSticksBlock(const BlockState &state) {
    return state.mName == "minecraft:slime" || state.mName == "minecraft:honey_block";
}

bool PistonSystem::isExtended(ServerNetworkHandler &owner, const Vector3i &position, const BlockState &state) {
    const int face = getPistonFace(state);
    const BlockState ahead = stateAt(owner, relative(position, face));
    return isArmCollision(ahead.mName) && getPistonFace(ahead) == face;
}

bool PistonSystem::isGettingPower(ServerNetworkHandler &owner, const Vector3i &position,
                                  const BlockState &state) {
    const int face = getPistonFace(state);

    for (int side = 0; side < RedstoneFace::COUNT; ++side) {
        if (side == face)
            continue;

        if (RedstoneSystem::isSidePowered(owner, relative(position, side), side))
            return true;
    }

    return false;
}

void PistonSystem::onRedstoneUpdate(ServerNetworkHandler &owner, const Vector3i &position,
                                    const BlockState &state) {
    _checkState(owner, position, state, isGettingPower(owner, position, state));
}

void PistonSystem::onBlockBroken(ServerNetworkHandler &owner, const Vector3i &position,
                                 const BlockState &state) {
    const int face = getPistonFace(state);
    const Vector3i armPosition = relative(position, face);
    const BlockState arm = stateAt(owner, armPosition);

    if (isArmCollision(arm.mName) && getPistonFace(arm) == face)
        RedstoneSystem::setBlockState(owner,armPosition, BlockState(AIR));
}

bool PistonSystem::_checkState(ServerNetworkHandler &owner, const Vector3i &position, const BlockState &state,
                               bool powered) {
    const PistonArmBlockActor *arm = BlockActorStore::getInstance().find<PistonArmBlockActor>(position);
    if (arm != nullptr && arm->isMoving())
        return false;

    const bool extended = isExtended(owner, position, state);

    if (powered && !extended)
        return _doMove(owner, position, state, true);
    if (!powered && extended)
        return _doMove(owner, position, state, false);

    return false;
}

bool PistonSystem::_doMove(ServerNetworkHandler &owner, const Vector3i &position, const BlockState &state,
                           bool extending) {
    const int face = getPistonFace(state);
    const bool sticky = isSticky(state.mName);
    const int moveDirection = extending ? face : RedstoneFace::opposite(face);
    const Vector3i armPosition = relative(position, face);

    std::vector<Vector3i> toMove;
    std::vector<Vector3i> toDestroy;

    if (extending || sticky) {
        const Vector3i origin = extending ? relative(position, face) : relative(position, face, 2);
        const BlockState originState = stateAt(owner, origin);

        if (!isAir(originState)) {
            const bool pushable = extending ? canBePushed(originState) : canBePulled(originState);
            if (!pushable) {
                if (extending)
                    return false;
            } else if (breaksWhenMoved(originState)) {
                if (extending || sticksToPiston(originState))
                    toDestroy.push_back(origin);
            } else {
                Vector3i current = origin;
                while (true) {
                    if ((int) toMove.size() >= MOVE_BLOCK_LIMIT) {
                        if (extending)
                            return false;
                        break;
                    }

                    toMove.push_back(current);

                    const Vector3i next = relative(current, moveDirection);
                    const BlockState nextState = stateAt(owner, next);

                    if (isAir(nextState) || (!extending && next == armPosition))
                        break;

                    if (!canBePushed(nextState) || next == position) {
                        if (extending)
                            return false;
                        break;
                    }

                    if (breaksWhenMoved(nextState)) {
                        toDestroy.push_back(next);
                        break;
                    }

                    current = next;
                }
            }
        }
    }

    for (size_t index = toDestroy.size(); index > 0; --index) {
        const Vector3i &destroyed = toDestroy[index - 1];
        RedstoneSystem::setBlockState(owner,destroyed, BlockState(AIR));
    }

    std::vector<BlockState> moved;
    moved.reserve(toMove.size());
    for (const Vector3i &source: toMove)
        moved.push_back(stateAt(owner, source));

    PistonArmBlockActor &arm = BlockActorStore::getInstance().getOrCreate<PistonArmBlockActor>(position);
    arm.setSticky(sticky);
    arm.setFacing(face);
    arm.beginMove(extending, toMove);

    for (size_t index = toMove.size(); index > 0; --index)
        RedstoneSystem::setBlockState(owner,toMove[index - 1], BlockState(AIR));

    gPendingMoves.push_back(PendingMove{position, moved, moveDirection});
    broadcastArmData(owner, arm);

    if (extending) {
        BlockState arm(sticky ? STICKY_PISTON_ARM_COLLISION : PISTON_ARM_COLLISION);
        arm.mStates = Tag::ofCompound();
        arm.mStates.putInt("facing_direction", getStoredFacing(state));
        RedstoneSystem::setBlockState(owner,armPosition, arm);
    } else if (isArmCollision(stateAt(owner, armPosition).mName)) {
        RedstoneSystem::setBlockState(owner,armPosition, BlockState(AIR));
    }

    const Vector3f center((float) position.x + 0.5f, (float) position.y + 0.5f, (float) position.z + 0.5f);
    owner.playLevelSound(extending ? LevelSoundEvent::PISTON_OUT : LevelSoundEvent::PISTON_IN, center);

    (void) faceAxis;
    return true;
}

void PistonSystem::tick(ServerNetworkHandler &owner) {
    if (gPendingMoves.empty())
        return;

    std::vector<PendingMove> pending;
    pending.swap(gPendingMoves);

    for (PendingMove &move: pending) {
        PistonArmBlockActor *arm = BlockActorStore::getInstance().find<PistonArmBlockActor>(move.mPiston);
        if (arm == nullptr)
            continue;

        arm->advance();

        const bool done = arm->isExtending() ? arm->getProgress() >= 1.0f : arm->getProgress() <= 0.0f;
        if (!done) {
            broadcastArmData(owner, *arm);
            gPendingMoves.push_back(std::move(move));
            continue;
        }

        const std::vector<Vector3i> attached = arm->getAttachedBlocks();
        for (size_t index = 0; index < attached.size() && index < move.mMoved.size(); ++index)
            RedstoneSystem::setBlockState(owner, relative(attached[index], move.mDirection),
                                          move.mMoved[index]);

        arm->finish();
        broadcastArmData(owner, *arm);

        for (const Vector3i &source: attached) {
            RedstoneSystem::updateAroundRedstone(owner, source);
            RedstoneSystem::updateAroundRedstone(owner, relative(source, move.mDirection));
        }
        RedstoneSystem::updateAroundRedstone(owner, relative(move.mPiston, arm->getFacing()));
    }
}
