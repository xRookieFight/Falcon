#include "Protocol/Packets/CameraAimAssistInstructionPacket.h"

#include "Protocol/NetworkPacketHandler.h"

CameraAimAssistInstructionPacket::CameraAimAssistInstructionPacket() = default;

void CameraAimAssistInstructionPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putString(mPresetId);
    stream.putByte((unsigned char) mAction);
    stream.putBool(mAllowAimAssist);
}

void CameraAimAssistInstructionPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mPresetId = stream.getString();
    mAction = (AimAssistAction) stream.getByte();
    mAllowAimAssist = stream.getBool();
}

void CameraAimAssistInstructionPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
