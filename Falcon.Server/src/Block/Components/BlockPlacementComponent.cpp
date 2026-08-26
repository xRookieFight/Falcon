#include "Block/Components/BlockPlacementComponent.h"

#include "Block/BlockData.h"
#include "Block/BlockIdentifier.h"
#include "Block/Blocks/DoorBlock.h"
#include "Level/Level.h"

#include <cmath>
#include <string>
#include <string_view>

namespace {
    constexpr int FACE_DOWN = 0;
    constexpr int FACE_UP = 1;
    constexpr int FACE_NORTH = 2;
    constexpr int FACE_SOUTH = 3;
    constexpr int FACE_WEST = 4;
    constexpr int FACE_EAST = 5;

    using BlockIdentifier::endsWith;
    using BlockIdentifier::endsWithAny;
    using BlockIdentifier::equalsAny;
    using BlockIdentifier::startsWith;

    bool isDoor(std::string_view identifier) {
        return endsWith(identifier, "_door");
    }

    bool isFenceGate(std::string_view identifier) {
        return identifier == "minecraft:fence_gate" || endsWith(identifier, "_fence_gate");
    }

    bool isCampfire(std::string_view identifier) {
        return identifier == "minecraft:campfire" || endsWith(identifier, "_campfire");
    }

    bool isBed(std::string_view identifier) {
        return identifier == "minecraft:bed" || endsWith(identifier, "_bed");
    }

    bool isTrapdoor(std::string_view identifier) {
        return identifier == "minecraft:trapdoor" || endsWith(identifier, "_trapdoor");
    }

    bool isBell(std::string_view identifier) {
        return identifier == "minecraft:bell";
    }

    bool isHopper(std::string_view identifier) {
        return identifier == "minecraft:hopper";
    }

    bool isPiston(std::string_view identifier) {
        return equalsAny(identifier, {"minecraft:piston", "minecraft:sticky_piston"});
    }

    bool isFacingMachine(std::string_view identifier) {
        return isPiston(identifier)
               || equalsAny(identifier, {"minecraft:dispenser", "minecraft:dropper", "minecraft:observer"});
    }

    bool isAmethystBud(std::string_view identifier) {
        return endsWithAny(identifier, {"_amethyst_bud", "_cluster"});
    }

    bool isShulkerBox(std::string_view identifier) {
        return endsWith(identifier, "_shulker_box") || identifier == "minecraft:undyed_shulker_box";
    }

    bool isMobHead(std::string_view identifier) {
        return equalsAny(identifier, {"minecraft:creeper_head", "minecraft:dragon_head",
                                      "minecraft:piglin_head", "minecraft:player_head",
                                      "minecraft:skeleton_skull", "minecraft:wither_skeleton_skull",
                                      "minecraft:zombie_head"});
    }

    bool isLadder(std::string_view identifier) {
        return identifier == "minecraft:ladder";
    }

    bool isWallMounted(std::string_view identifier) {
        return equalsAny(identifier, {"minecraft:wall_sign", "minecraft:wall_banner"})
               || endsWith(identifier, "_wall_sign");
    }

    bool isTorch(std::string_view identifier) {
        return identifier == "minecraft:torch"
               || startsWith(identifier, "minecraft:colored_torch_")
               || endsWith(identifier, "_torch");
    }

    bool isCoralFan(std::string_view identifier) {
        return endsWithAny(identifier, {"_coral_fan", "_coral_wall_fan"});
    }

    int horizontalFacing(float yaw) {
        float angle = std::fmod(yaw, 360.0f);
        if (angle < 0.0f)
            angle += 360.0f;

        if (angle < 45.0f || angle >= 315.0f)
            return FACE_SOUTH;
        if (angle < 135.0f)
            return FACE_WEST;
        if (angle < 225.0f)
            return FACE_NORTH;
        return FACE_EAST;
    }

    int horizontalOrdinal(int facing) {
        switch (facing) {
            case FACE_SOUTH:
                return 0;
            case FACE_WEST:
                return 1;
            case FACE_NORTH:
                return 2;
            default:
                return 3;
        }
    }

    const char *cardinalName(int facing) {
        switch (facing) {
            case FACE_NORTH:
                return "north";
            case FACE_SOUTH:
                return "south";
            case FACE_WEST:
                return "west";
            default:
                return "east";
        }
    }

    const char *pillarAxis(int face) {
        if (face == FACE_NORTH || face == FACE_SOUTH)
            return "z";
        if (face == FACE_WEST || face == FACE_EAST)
            return "x";
        return "y";
    }

    const char *faceName(int face) {
        switch (face) {
            case FACE_DOWN:
                return "down";
            case FACE_UP:
                return "up";
            case FACE_NORTH:
                return "north";
            case FACE_SOUTH:
                return "south";
            case FACE_WEST:
                return "west";
            default:
                return "east";
        }
    }

    Vector3i faceOffset(int face) {
        switch (face) {
            case FACE_DOWN:
                return Vector3i(0, -1, 0);
            case FACE_UP:
                return Vector3i(0, 1, 0);
            case FACE_NORTH:
                return Vector3i(0, 0, -1);
            case FACE_SOUTH:
                return Vector3i(0, 0, 1);
            case FACE_WEST:
                return Vector3i(-1, 0, 0);
            default:
                return Vector3i(1, 0, 0);
        }
    }

    Vector3i relativePosition(const Vector3i &position, int face) {
        const Vector3i offset = faceOffset(face);
        return Vector3i(position.x + offset.x, position.y + offset.y, position.z + offset.z);
    }

    bool isSolidAt(Level *level, const Vector3i &position) {
        if (level == nullptr)
            return false;

        const BlockState state = level->getBlockState(position.x, position.y, position.z);
        if (state.mName == "minecraft:air")
            return false;

        const BlockData *data = BlockDataTable::find(state.mName.c_str());
        return data != nullptr && data->mSolid;
    }

    int signRotation(float yaw) {
        const float value = ((yaw + 180.0f) * 16.0f / 360.0f) + 0.5f;
        int rotation = (int) std::floor(value) % 16;
        if (rotation < 0)
            rotation += 16;
        return rotation;
    }

    int cardinalFacing(std::string_view identifier, int playerFacing, int oppositeFacing) {
        if (isFenceGate(identifier) || isDoor(identifier) || isCampfire(identifier) || isBed(identifier))
            return playerFacing;

        return oppositeFacing;
    }

    const char *leverDirection(int face, int playerFacing) {
        if (face == FACE_DOWN)
            return playerFacing == FACE_WEST || playerFacing == FACE_EAST ? "down_east_west"
                                                                          : "down_north_south";

        if (face == FACE_UP)
            return playerFacing == FACE_WEST || playerFacing == FACE_EAST ? "up_east_west"
                                                                         : "up_north_south";

        return faceName(face);
    }

    const char *torchFacingDirection(int face) {
        switch (face) {
            case FACE_EAST:
                return "west";
            case FACE_WEST:
                return "east";
            case FACE_SOUTH:
                return "north";
            case FACE_NORTH:
                return "south";
            default:
                return "top";
        }
    }

    const float PLAYER_EYE_HEIGHT = 1.62f;

    int pistonFacingDirection(int playerFacing, const Vector3f &playerPosition, const Vector3i &blockPosition) {
        const int32_t playerBlockX = (int32_t) std::floor(playerPosition.x);
        const int32_t playerBlockZ = (int32_t) std::floor(playerPosition.z);

        if (std::abs(playerBlockX - blockPosition.x) > 1 || std::abs(playerBlockZ - blockPosition.z) > 1)
            return playerFacing;

        const float eyeY = playerPosition.y + PLAYER_EYE_HEIGHT;
        if (eyeY - (float) blockPosition.y > 2.0f)
            return FACE_UP;
        if ((float) blockPosition.y - eyeY > 0.0f)
            return FACE_DOWN;

        return playerFacing;
    }

    int facingDirection(std::string_view identifier, int oppositeFacing, int face) {
        if (isHopper(identifier))
            return face == FACE_DOWN ? FACE_DOWN : (face == FACE_UP ? FACE_DOWN : face ^ 1);

        if (isFacingMachine(identifier))
            return face == FACE_DOWN ? FACE_DOWN : face ^ 1;

        if (isAmethystBud(identifier) || isShulkerBox(identifier) || isMobHead(identifier))
            return face;

        if (isTorch(identifier))
            return face >= FACE_NORTH ? face : FACE_UP;

        if (isLadder(identifier) || isWallMounted(identifier) || isCoralFan(identifier)
            || isBell(identifier))
            return face >= FACE_NORTH ? face : oppositeFacing;

        return oppositeFacing;
    }

    const char *attachmentType(std::string_view identifier, Level *level, int face,
                               const Vector3i &blockPosition) {
        if (face == FACE_UP)
            return "standing";
        if (face == FACE_DOWN)
            return "hanging";

        if (isBell(identifier) && isSolidAt(level, relativePosition(blockPosition, face)))
            return "multiple";

        return "side";
    }

    std::string crafterOrientation(float pitch, int oppositeFacing) {
        const bool lookingDown = pitch > 45.0f;
        const bool lookingUp = pitch < -45.0f;

        if (lookingDown)
            return std::string("up_") + cardinalName(oppositeFacing);
        if (lookingUp)
            return std::string("down_") + cardinalName(oppositeFacing);

        return std::string(cardinalName(oppositeFacing)) + "_up";
    }
}

int BlockPlacementComponent::getHorizontalFacing(float yaw) {
    return horizontalFacing(yaw);
}

BlockState BlockPlacementComponent::apply(const BlockState &state, Level *level, float yaw, float pitch,
                                          int face, const Vector3f &clickPosition,
                                          const Vector3f &playerPosition, const Vector3i &blockPosition) {
    Tag states = state.mStates;
    const std::string_view identifier = state.mName;
    const int playerFacing = horizontalFacing(yaw);
    const int oppositeFacing = playerFacing ^ 1;
    const int ordinal = horizontalOrdinal(playerFacing);
    const int pistonFacing = pistonFacingDirection(playerFacing, playerPosition, blockPosition);

    if (states.contains("minecraft:cardinal_direction")) {
        const int facing = cardinalFacing(identifier, playerFacing, oppositeFacing);
        states.putString("minecraft:cardinal_direction", cardinalName(facing));
    }

    if (states.contains("minecraft:block_face"))
        states.putString("minecraft:block_face", faceName(face));

    if (states.contains("facing_direction")) {
        states.putInt("facing_direction", isPiston(identifier)
                                          ? pistonFacing
                                          : facingDirection(identifier, oppositeFacing, face));
    }

    if (states.contains("minecraft:facing_direction")) {
        states.putString("minecraft:facing_direction",
                         faceName(facingDirection(identifier, oppositeFacing, face)));
    }

    if (states.contains("lever_direction"))
        states.putString("lever_direction", leverDirection(face, playerFacing));

    if (states.contains("torch_facing_direction"))
        states.putString("torch_facing_direction", torchFacingDirection(face));

    if (states.contains("direction")) {
        int direction = ordinal;
        if (isTrapdoor(identifier))
            direction = horizontalOrdinal(oppositeFacing);
        else if (states.contains("attachment"))
            direction = face >= FACE_NORTH ? horizontalOrdinal(face) : horizontalOrdinal(oppositeFacing);
        states.putInt("direction", direction);
    }

    if (states.contains("attachment"))
        states.putString("attachment", attachmentType(identifier, level, face, blockPosition));

    if (states.contains("weirdo_direction"))
        states.putInt("weirdo_direction", ordinal * 2);

    if (states.contains("ground_sign_direction"))
        states.putInt("ground_sign_direction", signRotation(yaw));

    if (states.contains("coral_fan_direction"))
        states.putInt("coral_fan_direction", face >= FACE_NORTH ? face - FACE_NORTH : 0);

    if (states.contains("multi_face_direction_bits"))
        states.putInt("multi_face_direction_bits", 1 << face);

    if (states.contains("pillar_axis"))
        states.putString("pillar_axis", pillarAxis(face));

    if (states.contains("upside_down_bit")) {
        const bool upsideDown = face == FACE_DOWN
                                || (face != FACE_UP && clickPosition.y > 0.5f);
        states.putByte("upside_down_bit", upsideDown ? 1 : 0);
    }

    if (states.contains("minecraft:vertical_half")) {
        const bool top = face == FACE_DOWN
                         || (face != FACE_UP && clickPosition.y > 0.5f);
        states.putString("minecraft:vertical_half", top ? "top" : "bottom");
    }

    if (states.contains("hanging")) {
        const bool hanging = face == FACE_DOWN
                             || (face != FACE_UP && !isSolidAt(level, relativePosition(blockPosition, FACE_DOWN)));
        states.putByte("hanging", hanging ? 1 : 0);
    }

    if (states.contains("upper_block_bit"))
        states.putByte("upper_block_bit", 0);

    if (states.contains("head_piece_bit"))
        states.putByte("head_piece_bit", 0);

    if (states.contains("door_hinge_bit")) {
        const bool rightHinged = DoorBlock::isRightHinged(level, state.mName, blockPosition, playerFacing);
        states.putByte("door_hinge_bit", rightHinged ? 1 : 0);
    }

    if (states.contains("orientation"))
        states.putString("orientation", crafterOrientation(pitch, oppositeFacing));

    return BlockState(state.mName, states);
}
