#include "Protocol/Packets/CameraShakePacket.h"

#include "Protocol/NetworkPacketHandler.h"

CameraShakePacket::CameraShakePacket()
        : mIntensity(0.0f), mDuration(0.0f), mShakeType(ShakeType::Positional), mShakeAction(ShakeAction::Add) {}

void CameraShakePacket::write(BinaryStream &stream) const {
    stream.putLFloat(mIntensity);
    stream.putLFloat(mDuration);
    stream.putByte((unsigned char) mShakeType);
    stream.putByte((unsigned char) mShakeAction);
}

void CameraShakePacket::read(ReadOnlyBinaryStream &stream) {
    mIntensity = stream.getLFloat();
    mDuration = stream.getLFloat();
    mShakeType = (ShakeType) stream.getByte();
    mShakeAction = (ShakeAction) stream.getByte();
}

void CameraShakePacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
