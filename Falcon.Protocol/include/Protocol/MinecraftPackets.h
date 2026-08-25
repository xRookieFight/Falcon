#pragma once

#include "protocol/MinecraftPacketIds.h"
#include "protocol/Packet.h"

#include <functional>
#include <memory>

class MinecraftPackets {
public:
    typedef std::function<std::shared_ptr<Packet>()> PacketFactory;

    static std::shared_ptr<Packet> createPacket(MinecraftPacketIds id);

    static void registerPacket(MinecraftPacketIds id, const PacketFactory &factory);

    template<class PacketType>
    static void registerPacket() {
        registerPacket(PacketType::ID, []() { return std::static_pointer_cast<Packet>(
                std::make_shared<PacketType>()); });
    }

private:
    static void _registerDefaultPackets();
};
