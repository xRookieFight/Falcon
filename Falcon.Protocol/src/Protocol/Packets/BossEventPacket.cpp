#include "Protocol/Packets/BossEventPacket.h"

#include "Protocol/NetworkPacketHandler.h"

BossEventPacket::BossEventPacket()
        : mBossUniqueEntityId(0), mPlayerUniqueEntityId(0), mAction(Action::Create), mHealthPercentage(0.0f), mColor(0), mOverlay(0) {}

void BossEventPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putVarLong(mBossUniqueEntityId);
    stream.putVarLong(mPlayerUniqueEntityId);
    stream.putByte((unsigned char) mAction);
    stream.putString(mTitle);
    stream.putString(mFilteredTitle);
    stream.putLFloat(mHealthPercentage);
    stream.putByte((unsigned char) mColor);
    stream.putByte((unsigned char) mOverlay);
}

void BossEventPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mBossUniqueEntityId = stream.getVarLong();
    mPlayerUniqueEntityId = stream.getVarLong();
    mAction = (Action) stream.getByte();
    mTitle = stream.getString();
    mFilteredTitle = stream.getString();
    mHealthPercentage = stream.getLFloat();
    mColor = stream.getByte();
    mOverlay = stream.getByte();
}

void BossEventPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
