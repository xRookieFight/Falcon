#include "Protocol/Packets/CraftingDataPacket.h"

#include "Protocol/NetworkPacketHandler.h"

CraftingDataPacket::CraftingDataPacket() = default;

void CraftingDataPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    for (int i = 0; i < 11; i++) {
        stream.putArrayLength(0);
    }
    stream.putBool(mCleanRecipes);
}

void CraftingDataPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    for (int i = 0; i < 11; i++) {
        uint32_t length = stream.getArrayLength();
        for (uint32_t j = 0; j < length; j++) {
        }
    }
    mCleanRecipes = stream.getBool();
}

void CraftingDataPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
