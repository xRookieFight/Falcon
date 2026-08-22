#include "Network/MovementHandler.h"

#include "Entity/EntityFlags.h"
#include "Entity/ServerPlayer.h"
#include "Level/Level.h"
#include "Network/BadPacketCheck.h"
#include "Network/BlockActionHandler.h"
#include "Network/NetworkHandler.h"
#include "Network/ServerNetworkHandler.h"
#include "Protocol/Packets/MovePlayerPacket.h"
#include "Protocol/Packets/PlayerAuthInputPacket.h"
#include "Protocol/Types/StartGameTypes.h"

#include <cmath>

namespace {
    const float PLAYER_BASE_OFFSET = 1.62f;
    const float PLAYER_HALF_WIDTH = 0.3f;
    const float PLAYER_HEIGHT = 1.8f;
    const float GROUND_PROBE_DEPTH = 0.5f;
    const float MOVE_CORRECTION_EPSILON = 0.00001f;
    const float MAX_MOVE_DISTANCE_SQUARED = 225.0f;
    const float PLAYER_GRAVITY = 0.08f;
    const float PLAYER_VERTICAL_DRAG = 0.98f;
    const float PLAYER_JUMP_VELOCITY = 0.42f;
    const float PLAYER_HORIZONTAL_FRICTION = 0.91f;
    const float AABB_EPSILON = 0.001f;

    float sweepAxisX(ServerNetworkHandler &owner, const Vector3f &feet, float deltaX) {
        if (deltaX == 0.0f)
            return 0.0f;

        Level &level = owner.getLevel();
        const float minY = feet.y + AABB_EPSILON;
        const float maxY = feet.y + PLAYER_HEIGHT - AABB_EPSILON;
        const float minZ = feet.z - PLAYER_HALF_WIDTH + AABB_EPSILON;
        const float maxZ = feet.z + PLAYER_HALF_WIDTH - AABB_EPSILON;

        const float edge = deltaX > 0.0f ? feet.x + PLAYER_HALF_WIDTH : feet.x - PLAYER_HALF_WIDTH;
        const float targetEdge = edge + deltaX;

        const int32_t startX = (int32_t) std::floor(deltaX > 0.0f ? edge : targetEdge);
        const int32_t endX = (int32_t) std::floor(deltaX > 0.0f ? targetEdge : edge);

        float allowed = deltaX;

        for (int32_t x = startX; x <= endX; x++) {
            for (int32_t y = (int32_t) std::floor(minY); y <= (int32_t) std::floor(maxY); y++) {
                for (int32_t z = (int32_t) std::floor(minZ); z <= (int32_t) std::floor(maxZ); z++) {
                    if (!level.isSolidAt(x, y, z))
                        continue;

                    const float blockEdge = deltaX > 0.0f ? (float) x : (float) (x + 1);
                    const float candidate = blockEdge - edge;

                    if (deltaX > 0.0f && candidate < allowed)
                        allowed = candidate;
                    else if (deltaX < 0.0f && candidate > allowed)
                        allowed = candidate;
                }
            }
        }

        return allowed;
    }

    float sweepAxisY(ServerNetworkHandler &owner, const Vector3f &feet, float deltaY) {
        if (deltaY == 0.0f)
            return 0.0f;

        Level &level = owner.getLevel();
        const float minX = feet.x - PLAYER_HALF_WIDTH + AABB_EPSILON;
        const float maxX = feet.x + PLAYER_HALF_WIDTH - AABB_EPSILON;
        const float minZ = feet.z - PLAYER_HALF_WIDTH + AABB_EPSILON;
        const float maxZ = feet.z + PLAYER_HALF_WIDTH - AABB_EPSILON;

        const float edge = deltaY > 0.0f ? feet.y + PLAYER_HEIGHT : feet.y;
        const float targetEdge = edge + deltaY;

        const int32_t startY = (int32_t) std::floor(deltaY > 0.0f ? edge : targetEdge);
        const int32_t endY = (int32_t) std::floor(deltaY > 0.0f ? targetEdge : edge);

        float allowed = deltaY;

        for (int32_t y = startY; y <= endY; y++) {
            for (int32_t x = (int32_t) std::floor(minX); x <= (int32_t) std::floor(maxX); x++) {
                for (int32_t z = (int32_t) std::floor(minZ); z <= (int32_t) std::floor(maxZ); z++) {
                    if (!level.isSolidAt(x, y, z))
                        continue;

                    const float blockEdge = deltaY > 0.0f ? (float) y : (float) (y + 1);
                    const float candidate = blockEdge - edge;

                    if (deltaY > 0.0f && candidate < allowed)
                        allowed = candidate;
                    else if (deltaY < 0.0f && candidate > allowed)
                        allowed = candidate;
                }
            }
        }

        return allowed;
    }

    float sweepAxisZ(ServerNetworkHandler &owner, const Vector3f &feet, float deltaZ) {
        if (deltaZ == 0.0f)
            return 0.0f;

        Level &level = owner.getLevel();
        const float minY = feet.y + AABB_EPSILON;
        const float maxY = feet.y + PLAYER_HEIGHT - AABB_EPSILON;
        const float minX = feet.x - PLAYER_HALF_WIDTH + AABB_EPSILON;
        const float maxX = feet.x + PLAYER_HALF_WIDTH - AABB_EPSILON;

        const float edge = deltaZ > 0.0f ? feet.z + PLAYER_HALF_WIDTH : feet.z - PLAYER_HALF_WIDTH;
        const float targetEdge = edge + deltaZ;

        const int32_t startZ = (int32_t) std::floor(deltaZ > 0.0f ? edge : targetEdge);
        const int32_t endZ = (int32_t) std::floor(deltaZ > 0.0f ? targetEdge : edge);

        float allowed = deltaZ;

        for (int32_t z = startZ; z <= endZ; z++) {
            for (int32_t x = (int32_t) std::floor(minX); x <= (int32_t) std::floor(maxX); x++) {
                for (int32_t y = (int32_t) std::floor(minY); y <= (int32_t) std::floor(maxY); y++) {
                    if (!level.isSolidAt(x, y, z))
                        continue;

                    const float blockEdge = deltaZ > 0.0f ? (float) z : (float) (z + 1);
                    const float candidate = blockEdge - edge;

                    if (deltaZ > 0.0f && candidate < allowed)
                        allowed = candidate;
                    else if (deltaZ < 0.0f && candidate > allowed)
                        allowed = candidate;
                }
            }
        }

        return allowed;
    }

    Vector3f simulatePlayerMovement(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3f &wantedDelta,
                                    bool jumping, bool &outJumped) {
        const bool noPhysics = player.isFlying() || player.getGameType() == (int32_t) GameType::Spectator;

        Vector3f velocity = player.getMotion();
        outJumped = false;

        if (noPhysics) {
            velocity = wantedDelta;
        } else {
            velocity.x = wantedDelta.x;
            velocity.z = wantedDelta.z;

            if (player.isOnGround() && jumping) {
                velocity.y = PLAYER_JUMP_VELOCITY;
                outJumped = true;
            } else {
                velocity.y -= PLAYER_GRAVITY;
            }

            velocity.y *= PLAYER_VERTICAL_DRAG;
        }

        const Vector3f feet = player.getPosition();

        const float allowedY = noPhysics ? velocity.y : sweepAxisY(owner, feet, velocity.y);
        Vector3f afterY(feet.x, feet.y + allowedY, feet.z);

        const float allowedX = noPhysics ? velocity.x : sweepAxisX(owner, afterY, velocity.x);
        Vector3f afterX(afterY.x + allowedX, afterY.y, afterY.z);

        const float allowedZ = noPhysics ? velocity.z : sweepAxisZ(owner, afterX, velocity.z);
        Vector3f afterZ(afterX.x, afterX.y, afterX.z + allowedZ);

        if (!noPhysics) {
            if (allowedY != velocity.y)
                velocity.y = 0.0f;

            if (allowedX != velocity.x)
                velocity.x = 0.0f;

            if (allowedZ != velocity.z)
                velocity.z = 0.0f;

            velocity.x *= PLAYER_HORIZONTAL_FRICTION;
            velocity.z *= PLAYER_HORIZONTAL_FRICTION;
        }

        player.setMotion(velocity);
        return afterZ;
    }

    void sendMovementCorrection(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3f &feet) {
        MovePlayerPacket correction;
        correction.mRuntimeEntityId = (int64_t) player.getRuntimeId();
        correction.mPosition = Vector3f(feet.x, feet.y + PLAYER_BASE_OFFSET, feet.z);
        correction.mRotation = player.getRotation();
        correction.mMode = MovePlayerMode::Respawn;
        correction.mOnGround = player.isOnGround();

        owner.getNetworkHandler().send(player.getNetworkIdentifier(), correction, owner.getCodecContext());
    }

    void revertMovement(ServerNetworkHandler &owner, ServerPlayer &player) {
        const Vector3f position = player.getPosition();

        MovePlayerPacket revert;
        revert.mRuntimeEntityId = (int64_t) player.getRuntimeId();
        revert.mPosition = Vector3f(position.x, position.y + PLAYER_BASE_OFFSET + MOVE_CORRECTION_EPSILON,
                                    position.z);
        revert.mRotation = player.getRotation();
        revert.mMode = MovePlayerMode::Respawn;
        revert.mOnGround = player.isOnGround();

        owner.getNetworkHandler().send(player.getNetworkIdentifier(), revert, owner.getCodecContext());
        player.setForceMoveSync(true);
    }
}

bool MovementHandler::checkGroundState(ServerNetworkHandler &owner, const Vector3f &feetPosition) {
    const float shrink = 0.01f;
    const float minX = feetPosition.x - PLAYER_HALF_WIDTH + shrink;
    const float maxX = feetPosition.x + PLAYER_HALF_WIDTH - shrink;
    const float minZ = feetPosition.z - PLAYER_HALF_WIDTH + shrink;
    const float maxZ = feetPosition.z + PLAYER_HALF_WIDTH - shrink;

    Level &level = owner.getLevel();
    const int32_t lowestY = (int32_t) std::floor(feetPosition.y - GROUND_PROBE_DEPTH);
    const int32_t highestY = (int32_t) std::floor(feetPosition.y);

    for (int32_t y = lowestY; y <= highestY; y++) {
        for (int32_t x = (int32_t) std::floor(minX); x <= (int32_t) std::floor(maxX); x++) {
            for (int32_t z = (int32_t) std::floor(minZ); z <= (int32_t) std::floor(maxZ); z++) {
                if (level.isSolidAt(x, y, z))
                    return true;
            }
        }
    }

    return false;
}

void MovementHandler::handleMovement(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3f &feetPosition,
                                     const Vector3f &rotation) {
    const Vector3f previous = player.getPosition();

    const float requestedX = feetPosition.x - previous.x;
    const float requestedY = feetPosition.y - previous.y;
    const float requestedZ = feetPosition.z - previous.z;
    const float distanceSquared = requestedX * requestedX + requestedY * requestedY + requestedZ * requestedZ;

    player.setRotation(rotation);

    if (distanceSquared > MAX_MOVE_DISTANCE_SQUARED) {
        player.setMotion(Vector3f(0.0f, 0.0f, 0.0f));
        player.resetFallDistance();
        sendMovementCorrection(owner, player, previous);
        return;
    }

    if (player.getFlags().get(EntityFlag::Sprinting)) {
        const float horizontalDistance = std::sqrt(requestedX * requestedX + requestedZ * requestedZ);
        if (horizontalDistance > 0.0f)
            player.exhaust(0.01f * horizontalDistance);
    }

    player.setPosition(feetPosition);

    const bool wasOnGround = player.isOnGround();
    const bool onGround = checkGroundState(owner, feetPosition);
    player.setOnGround(onGround);

    if (onGround) {
        if (!wasOnGround)
            owner._handleFallDamage(player);

        player.resetFallDistance();
    } else {
        if (feetPosition.y > player.getHighestPosition())
            player.setHighestPosition(feetPosition.y);

        player.updateFallDistance();
    }
}

void MovementHandler::handlePlayerAuthInput(ServerNetworkHandler &owner, const NetworkIdentifier &id,
                                            ServerPlayer &player, const PlayerAuthInputPacket &packet) {
    const Vector3f feetPosition(packet.mPosition.x, packet.mPosition.y - PLAYER_BASE_OFFSET, packet.mPosition.z);

    const bool jumping = packet.hasInputFlag((int32_t) PlayerAuthInputData::StartJumping)
                        || packet.hasInputFlag((int32_t) PlayerAuthInputData::Jumping);

    player.queueMove(feetPosition, packet.mRotation, (uint64_t) packet.mTick, jumping);

    if (player.isSpawned()) {
        const int32_t chunkX = (int32_t) std::floor(feetPosition.x) >> 4;
        const int32_t chunkZ = (int32_t) std::floor(feetPosition.z) >> 4;

        if (!player.hasChunkPosition() || chunkX != player.getLastChunkX() || chunkZ != player.getLastChunkZ())
            owner._sendChunks(player);
    }

    EntityFlags &flags = player.getFlags();
    const EntityFlags previous = flags;

    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StartSprinting))
        flags.set(EntityFlag::Sprinting, true);
    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StopSprinting))
        flags.set(EntityFlag::Sprinting, false);

    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StartSneaking))
        flags.set(EntityFlag::Sneaking, true);
    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StopSneaking))
        flags.set(EntityFlag::Sneaking, false);

    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StartSwimming))
        flags.set(EntityFlag::Swimming, true);
    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StopSwimming))
        flags.set(EntityFlag::Swimming, false);

    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StartGliding))
        flags.set(EntityFlag::Gliding, true);
    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StopGliding))
        flags.set(EntityFlag::Gliding, false);

    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StartCrawling))
        flags.set(EntityFlag::Crawling, true);
    if (packet.hasInputFlag((int32_t) PlayerAuthInputData::StopCrawling))
        flags.set(EntityFlag::Crawling, false);

    if (flags.getLowBits() != previous.getLowBits() || flags.getHighBits() != previous.getHighBits())
        owner._sendEntityData(player);

    for (const PlayerBlockActionData &action: packet.mPlayerActions) {
        if (action.mAction == PlayerActionType::StartBreak ||
            action.mAction == PlayerActionType::BlockContinueDestroy) {
            BlockActionHandler::startBreakingBlock(owner, player, action.mBlockPosition, action.mFace);
        } else if (action.mAction == PlayerActionType::AbortBreak ||
                  action.mAction == PlayerActionType::StopBreak) {
            BlockActionHandler::stopBreakingBlock(owner, player);
        } else if (action.mAction == PlayerActionType::BlockPredictDestroy ||
                  action.mAction == PlayerActionType::DimensionChangeRequestOrCreativeDestroyBlock) {
            BlockActionHandler::completeBreakingBlock(owner, player, action.mBlockPosition);
        } else if (action.mAction == PlayerActionType::Jump) {
            player.exhaust(player.getFlags().get(EntityFlag::Sprinting) ? 0.2f : 0.05f);
        }
    }

    if (packet.mHasItemUseTransaction) {
        if (packet.mItemUseTransaction.mActionType == 0)
            BlockActionHandler::placeBlock(owner, player, packet.mItemUseTransaction);
        else if (packet.mItemUseTransaction.mActionType == 1)
            owner._useHeldItem(player);
    }

    (void) id;
    (void) revertMovement;
    (void) simulatePlayerMovement;
}
