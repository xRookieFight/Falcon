#include "protocol/packets/ClientboundUpdateSoundDataPacket.h"

#include "protocol/NetworkPacketHandler.h"

ClientboundUpdateSoundDataPacket::ClientboundUpdateSoundDataPacket() = default;

void ClientboundUpdateSoundDataPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putLLong((uint64_t) mServerSoundHandle);

    stream.putOptionalPresent(mHasStop);
    if (mHasStop) {
        stream.putUnsignedVarInt(0);
    }

    stream.putOptionalPresent(mHasVolume);
    if (mHasVolume) {
        stream.putUnsignedVarInt(0);
        stream.putLFloat(mVolume.mVolume);
    }

    stream.putOptionalPresent(mHasPitch);
    if (mHasPitch) {
        stream.putUnsignedVarInt(0);
        stream.putLFloat(mPitch.mPitch);
    }

    stream.putOptionalPresent(mHasFade);
    if (mHasFade) {
        stream.putUnsignedVarInt(0);
        stream.putLFloat(mFade.mTargetVolume);
        stream.putLFloat(mFade.mDuration);
    }

    stream.putOptionalPresent(mHasSeekTo);
    if (mHasSeekTo) {
        stream.putUnsignedVarInt(0);
        stream.putLFloat(mSeekTo.mSeconds);
    }

    stream.putOptionalPresent(mHasPause);
    if (mHasPause) {
        stream.putUnsignedVarInt(0);
    }

    stream.putOptionalPresent(mHasResume);
    if (mHasResume) {
        stream.putUnsignedVarInt(0);
    }
}

void ClientboundUpdateSoundDataPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mServerSoundHandle = (int64_t) stream.getLLong();

    mHasStop = stream.getOptionalPresent();
    if (mHasStop) {
        stream.getUnsignedVarInt();
    }

    mHasVolume = stream.getOptionalPresent();
    if (mHasVolume) {
        stream.getUnsignedVarInt();
        mVolume.mVolume = stream.getLFloat();
    }

    mHasPitch = stream.getOptionalPresent();
    if (mHasPitch) {
        stream.getUnsignedVarInt();
        mPitch.mPitch = stream.getLFloat();
    }

    mHasFade = stream.getOptionalPresent();
    if (mHasFade) {
        stream.getUnsignedVarInt();
        mFade.mTargetVolume = stream.getLFloat();
        mFade.mDuration = stream.getLFloat();
    }

    mHasSeekTo = stream.getOptionalPresent();
    if (mHasSeekTo) {
        stream.getUnsignedVarInt();
        mSeekTo.mSeconds = stream.getLFloat();
    }

    mHasPause = stream.getOptionalPresent();
    if (mHasPause) {
        stream.getUnsignedVarInt();
    }

    mHasResume = stream.getOptionalPresent();
    if (mHasResume) {
        stream.getUnsignedVarInt();
    }
}

void ClientboundUpdateSoundDataPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
