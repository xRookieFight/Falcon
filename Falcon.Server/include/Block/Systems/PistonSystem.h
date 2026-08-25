#pragma once

#include "Block/BlockState.h"
#include "Core/Math/Vector3i.h"

#include <string>
#include <vector>

class ServerNetworkHandler;

class PistonSystem {
public:
    static const int MOVE_BLOCK_LIMIT = 12;

    static bool isPiston(const std::string &identifier);

    static bool isSticky(const std::string &identifier);

    static bool isArmCollision(const std::string &identifier);

    static int getPistonFace(const BlockState &state);

    static int getStoredFacing(const BlockState &state);

    static bool isExtended(ServerNetworkHandler &owner, const Vector3i &position, const BlockState &state);

    static bool isGettingPower(ServerNetworkHandler &owner, const Vector3i &position, const BlockState &state);

    static void onRedstoneUpdate(ServerNetworkHandler &owner, const Vector3i &position, const BlockState &state);

    static void tick(ServerNetworkHandler &owner);

    static void onBlockBroken(ServerNetworkHandler &owner, const Vector3i &position, const BlockState &state);

    static bool canBePushed(const BlockState &state);

    static bool canBePulled(const BlockState &state);

    static bool breaksWhenMoved(const BlockState &state);

    static bool sticksToPiston(const BlockState &state);

    static bool canSticksBlock(const BlockState &state);

private:
    static bool _checkState(ServerNetworkHandler &owner, const Vector3i &position, const BlockState &state,
                            bool powered);

    static bool _doMove(ServerNetworkHandler &owner, const Vector3i &position, const BlockState &state,
                        bool extending);
};
