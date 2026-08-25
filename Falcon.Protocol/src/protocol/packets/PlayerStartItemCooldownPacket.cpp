#include "protocol/packets/PlayerStartItemCooldownPacket.h"

#include "protocol/NetworkPacketHandler.h"

PlayerStartItemCooldownPacket::PlayerStartItemCooldownPacket() = default;

void PlayerStartItemCooldownPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putString(mItemCategory);
    stream.putVarInt(mCooldownDuration);
}

void PlayerStartItemCooldownPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mItemCategory = stream.getString();
    mCooldownDuration = stream.getVarInt();
}

void PlayerStartItemCooldownPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
