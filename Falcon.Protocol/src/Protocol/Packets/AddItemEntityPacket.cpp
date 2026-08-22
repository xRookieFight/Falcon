#include "Protocol/Packets/AddItemEntityPacket.h"

#include "Protocol/EntityCodec.h"
#include "Protocol/ItemCodec.h"
#include "Protocol/NetworkPacketHandler.h"

AddItemEntityPacket::AddItemEntityPacket() = default;

void AddItemEntityPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putVarLong(mUniqueEntityId);
    stream.putUnsignedVarLong(mRuntimeEntityId);
    ItemCodec::writeNetworkItemStackDescriptor(stream, context, mItemInHand);
    stream.putVector3f(mPosition);
    stream.putVector3f(mMotion);
    EntityCodec::writeEntityData(stream, mMetadata);
    stream.putBool(mFromFishing);
}

void AddItemEntityPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mUniqueEntityId = stream.getVarLong();
    mRuntimeEntityId = stream.getUnsignedVarLong();
    mItemInHand = ItemCodec::readNetworkItemStackDescriptor(stream, context);
    mPosition = stream.getVector3f();
    mMotion = stream.getVector3f();
    mMetadata = EntityCodec::readEntityData(stream);
    mFromFishing = stream.getBool();
}

void AddItemEntityPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
