#pragma once

#include "Core/Math/Vector3i.h"

#include <cstdint>
#include <string>

class ServerNetworkHandler;
class ServerPlayer;
class CommandBlockActor;
class CommandBlockUpdatePacket;

class CommandBlockSystem {
public:
    static int64_t packPosition(const Vector3i &position);

    static CommandBlockActor *find(const Vector3i &position);

    static CommandBlockActor &getOrCreate(ServerNetworkHandler &owner, const Vector3i &position);

    static void remove(const Vector3i &position);

    static void onCommandBlockUpdate(ServerNetworkHandler &owner, ServerPlayer &player,
                                     const CommandBlockUpdatePacket &packet);


    static void setPowered(ServerNetworkHandler &owner, const Vector3i &position, bool powered);

    static void trigger(ServerNetworkHandler &owner, const Vector3i &position, int chain);

    static void broadcastData(ServerNetworkHandler &owner, const CommandBlockActor &actor);

    static void tickCommandBlocks(ServerNetworkHandler &owner);
};
