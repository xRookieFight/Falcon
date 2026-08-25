#include "protocol/packets/StopSoundPacket.h"

#include "protocol/NetworkPacketHandler.h"

StopSoundPacket::StopSoundPacket()
        : mStoppingAllSound(false), mStopMusicLegacy(false) {}

void StopSoundPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putString(mSoundName);
    stream.putBool(mStoppingAllSound);
    stream.putBool(mStopMusicLegacy);
}

void StopSoundPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mSoundName = stream.getString();
    mStoppingAllSound = stream.getBool();
    mStopMusicLegacy = stream.getBool();
}

void StopSoundPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
