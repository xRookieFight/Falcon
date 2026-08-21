#include "Protocol/Packets/MovementPredictionSyncPacket.h"

#include "Protocol/NetworkPacketHandler.h"
#include "Protocol/Types/EntityFlagSet.h"

MovementPredictionSyncPacket::MovementPredictionSyncPacket() = default;

void MovementPredictionSyncPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    EntityFlagCodec::writeFlags(stream, mFlags);
    stream.putVector3f(mBoundingBox);
    stream.putLFloat(mSpeed);
    stream.putLFloat(mUnderwaterSpeed);
    stream.putLFloat(mLavaSpeed);
    stream.putLFloat(mJumpStrength);
    stream.putLFloat(mHealth);
    stream.putLFloat(mHunger);
    stream.putLFloat(mUnknown1);
    stream.putLFloat(mUnknown2);
    stream.putLFloat(mUnknown3);
    stream.putUnsignedVarLong(mRuntimeEntityId);
    stream.putBool(mFlying);
}

void MovementPredictionSyncPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mFlags = EntityFlagCodec::readFlags(stream);
    mBoundingBox = stream.getVector3f();
    mSpeed = stream.getLFloat();
    mUnderwaterSpeed = stream.getLFloat();
    mLavaSpeed = stream.getLFloat();
    mJumpStrength = stream.getLFloat();
    mHealth = stream.getLFloat();
    mHunger = stream.getLFloat();
    mUnknown1 = stream.getLFloat();
    mUnknown2 = stream.getLFloat();
    mUnknown3 = stream.getLFloat();
    mRuntimeEntityId = stream.getUnsignedVarLong();
    mFlying = stream.getBool();
}

void MovementPredictionSyncPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
