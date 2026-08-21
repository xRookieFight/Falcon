#pragma once

#include "Protocol/Packet.h"
#include "Protocol/Types/SoundDataTypes.h"

#include <cstdint>

class ClientboundUpdateSoundDataPacket : public Packet {
public:
    static const MinecraftPacketIds ID = MinecraftPacketIds::ClientboundUpdateSoundData;

    ClientboundUpdateSoundDataPacket();

    MinecraftPacketIds getId() const override { return ID; }

    const char *getName() const override { return "ClientboundUpdateSoundDataPacket"; }

    void write(BinaryStream &stream, const PacketCodecContext &context) const override;

    void read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) override;

    void handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const override;

    int64_t mServerSoundHandle = 0;

    bool mHasStop = false;

    bool mHasVolume = false;
    SetVolumeSoundData mVolume;

    bool mHasPitch = false;
    SetPitchSoundData mPitch;

    bool mHasFade = false;
    FadeSoundData mFade;

    bool mHasSeekTo = false;
    SeekToSoundData mSeekTo;

    bool mHasPause = false;

    bool mHasResume = false;
};
