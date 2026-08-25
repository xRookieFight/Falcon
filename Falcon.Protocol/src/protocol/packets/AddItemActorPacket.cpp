#include "protocol/packets/AddItemActorPacket.h"

#include "protocol/EntityCodec.h"
#include "protocol/ItemCodec.h"
#include "protocol/NetworkPacketHandler.h"

AddItemActorPacket::AddItemActorPacket() = default;

void AddItemActorPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putVarLong(mUniqueActorId);
    stream.putUnsignedVarLong(mRuntimeActorId);
    ItemCodec::writeNetworkItemStackDescriptor(stream, context, mItemInHand);
    stream.putVector3f(mPosition);
    stream.putVector3f(mMotion);
    EntityCodec::writeEntityData(stream, mMetadata);
    stream.putBool(mFromFishing);
}

void AddItemActorPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mUniqueActorId = stream.getVarLong();
    mRuntimeActorId = stream.getUnsignedVarLong();
    mItemInHand = ItemCodec::readNetworkItemStackDescriptor(stream, context);
    mPosition = stream.getVector3f();
    mMotion = stream.getVector3f();
    mMetadata = EntityCodec::readEntityData(stream);
    mFromFishing = stream.getBool();
}

void AddItemActorPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
