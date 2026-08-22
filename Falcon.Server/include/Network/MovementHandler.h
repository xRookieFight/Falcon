#pragma once

#include "Core/Math/Vector3f.h"

class ServerNetworkHandler;
class ServerPlayer;
class NetworkIdentifier;
class PlayerAuthInputPacket;

class MovementHandler {
public:
    static void handlePlayerAuthInput(ServerNetworkHandler &owner, const NetworkIdentifier &id, ServerPlayer &player,
                                      const PlayerAuthInputPacket &packet);

    static void handleMovement(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3f &feetPosition,
                               const Vector3f &rotation);

    static bool checkGroundState(ServerNetworkHandler &owner, const Vector3f &feetPosition);
};
