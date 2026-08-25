#include "Protocol/Packets/PlayerSkinPacket.h"

#include "Protocol/NetworkPacketHandler.h"
#include "Protocol/SkinCodec.h"

PlayerSkinPacket::PlayerSkinPacket() = default;

void PlayerSkinPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putUuid(mUuid);
    SkinCodec::writeSkin(stream, mSkin);
    stream.putString(mNewSkinName);
    stream.putString(mOldSkinName);
    stream.putBool(mTrustedSkin);
}

void PlayerSkinPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mUuid = stream.getUuid();
    mSkin = SkinCodec::readSkin(stream);
    mNewSkinName = stream.getString();
    mOldSkinName = stream.getString();

    if (!stream.feof()) {
        mTrustedSkin = stream.getBool();
    }
}

void PlayerSkinPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
