#include "Protocol/Packets/MovePlayerPacket.h"

#include "Protocol/NetworkPacketHandler.h"

MovePlayerPacket::MovePlayerPacket() = default;

void MovePlayerPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putUnsignedVarLong((uint64_t) mRuntimeEntityId);
    stream.putVector3f(mPosition);
    stream.putVector3f(mRotation);
    stream.putByte((unsigned char) mMode);
    stream.putBool(mOnGround);
    stream.putUnsignedVarLong((uint64_t) mRidingRuntimeEntityId);

    bool isTeleport = mMode == MovePlayerMode::Teleport;
    stream.putBool(isTeleport);
    if (isTeleport) {
        stream.putLInt((uint32_t) mTeleportationCause);
        stream.putLInt((uint32_t) mEntityType);
    }

    stream.putUnsignedVarLong((uint64_t) mTick);
}

void MovePlayerPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mRuntimeEntityId = (int64_t) stream.getUnsignedVarLong();
    mPosition = stream.getVector3f();
    mRotation = stream.getVector3f();
    mMode = (MovePlayerMode) stream.getByte();
    mOnGround = stream.getBool();
    mRidingRuntimeEntityId = (int64_t) stream.getUnsignedVarLong();

    mHasTeleportData = stream.getBool();
    if (mHasTeleportData) {
        mTeleportationCause = (MovePlayerTeleportationCause) stream.getLInt();
        mEntityType = (int32_t) stream.getLInt();
    }

    mTick = (int64_t) stream.getUnsignedVarLong();
}

void MovePlayerPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
