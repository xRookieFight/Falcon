#include "Block/Components/BlockPlacementComponent.h"

#include <cmath>
#include <string_view>

namespace {
    constexpr int FACE_DOWN = 0;
    constexpr int FACE_UP = 1;
    constexpr int FACE_NORTH = 2;
    constexpr int FACE_SOUTH = 3;
    constexpr int FACE_WEST = 4;
    constexpr int FACE_EAST = 5;

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

    int signRotation(float yaw) {
        const float value = ((yaw + 180.0f) * 16.0f / 360.0f) + 0.5f;
        int rotation = (int) std::floor(value) % 16;
        if (rotation < 0)
            rotation += 16;
        return rotation;
    }

    int cardinalFacing(std::string_view identifier, int playerFacing, int oppositeFacing) {
        if (identifier.find("fence_gate") != std::string_view::npos
            || identifier.find("_door") != std::string_view::npos
            || identifier.find("campfire") != std::string_view::npos
            || identifier.find("bed") != std::string_view::npos)
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

    bool isPistonLike(std::string_view identifier) {
        return identifier.find("piston") != std::string_view::npos;
    }

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
        if (identifier.find("hopper") != std::string_view::npos)
            return face == FACE_DOWN ? FACE_DOWN : (face == FACE_UP ? FACE_DOWN : face ^ 1);

        if (identifier.find("piston") != std::string_view::npos
            || identifier.find("dispenser") != std::string_view::npos
            || identifier.find("dropper") != std::string_view::npos
            || identifier.find("observer") != std::string_view::npos)
            return face == FACE_DOWN ? FACE_DOWN : face ^ 1;

        if (identifier.find("amethyst") != std::string_view::npos
            || identifier.find("shulker") != std::string_view::npos
            || identifier.find("head") != std::string_view::npos)
            return face;

        if (identifier.find("ladder") != std::string_view::npos
            || identifier.find("wall") != std::string_view::npos
            || identifier.find("torch") != std::string_view::npos
            || identifier.find("coral_fan") != std::string_view::npos)
            return face >= FACE_NORTH ? face : (identifier.find("torch") != std::string_view::npos ? FACE_UP : oppositeFacing);

        if (identifier.find("bell") != std::string_view::npos)
            return face >= FACE_NORTH ? face : oppositeFacing;

        return oppositeFacing;
    }
}

BlockState BlockPlacementComponent::apply(const BlockState &state, float yaw, int face,
                                          const Vector3f &clickPosition, const Vector3f &playerPosition,
                                          const Vector3i &blockPosition) {
    Tag states = state.mStates;
    const int playerFacing = horizontalFacing(yaw);
    const int oppositeFacing = playerFacing ^ 1;
    const int ordinal = horizontalOrdinal(playerFacing);
    const int pistonFacing = pistonFacingDirection(playerFacing, playerPosition, blockPosition);

    if (states.contains("minecraft:cardinal_direction")) {
        const int facing = cardinalFacing(state.mName, playerFacing, oppositeFacing);
        states.putString("minecraft:cardinal_direction", cardinalName(facing));
    }

    if (states.contains("minecraft:block_face"))
        states.putString("minecraft:block_face", faceName(face));

    if (states.contains("facing_direction")) {
        states.putInt("facing_direction", isPistonLike(state.mName)
                                          ? pistonFacing
                                          : facingDirection(state.mName, oppositeFacing, face));
    }

    if (states.contains("minecraft:facing_direction")) {
        states.putString("minecraft:facing_direction",
                         faceName(facingDirection(state.mName, oppositeFacing, face)));
    }

    if (states.contains("lever_direction"))
        states.putString("lever_direction", leverDirection(face, playerFacing));

    if (states.contains("torch_facing_direction"))
        states.putString("torch_facing_direction", torchFacingDirection(face));

    if (states.contains("direction")) {
        int direction = ordinal;
        if (state.mName.find("trapdoor") != std::string::npos)
            direction = horizontalOrdinal(oppositeFacing);
        else if (state.mName.find("bell") != std::string::npos)
            direction = face >= FACE_NORTH ? horizontalOrdinal(face) : horizontalOrdinal(oppositeFacing);
        states.putInt("direction", direction);
    }

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

    return BlockState(state.mName, states);
}
