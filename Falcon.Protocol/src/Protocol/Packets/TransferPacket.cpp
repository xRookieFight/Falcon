#include "Protocol/Packets/TransferPacket.h"

#include "Protocol/NetworkPacketHandler.h"

TransferPacket::TransferPacket() = default;

void TransferPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putString(mAddress);
    stream.putLShort((uint16_t) mPort);
    stream.putBool(mReloadWorld);

    stream.putOptionalPresent(mHasGatheringsConfigurationJoinInfo);
    if (!mHasGatheringsConfigurationJoinInfo)
        return;

    const GatheringsConfigurationJoinInfo &info = mGatheringsConfigurationJoinInfo;
    stream.putUuid(info.mExperienceId);
    stream.putString(info.mExperienceName);

    stream.putOptionalPresent(info.mHasWorldId);
    if (info.mHasWorldId)
        stream.putUuid(info.mWorldId);

    stream.putOptionalPresent(info.mHasWorldName);
    if (info.mHasWorldName)
        stream.putString(info.mWorldName);

    stream.putString(info.mCreatorId);

    stream.putOptionalPresent(info.mHasTargetId);
    if (info.mHasTargetId)
        stream.putUuid(info.mTargetId);

    stream.putOptionalPresent(info.mHasScenarioId);
    if (info.mHasScenarioId)
        stream.putString(info.mScenarioId);

    stream.putOptionalPresent(info.mHasServerId);
    if (info.mHasServerId)
        stream.putString(info.mServerId);
}

void TransferPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mAddress = stream.getString();
    mPort = stream.getLShort();
    mReloadWorld = stream.getBool();

    mHasGatheringsConfigurationJoinInfo = stream.getOptionalPresent();
    if (!mHasGatheringsConfigurationJoinInfo)
        return;

    GatheringsConfigurationJoinInfo &info = mGatheringsConfigurationJoinInfo;
    info.mExperienceId = stream.getUuid();
    info.mExperienceName = stream.getString();

    info.mHasWorldId = stream.getOptionalPresent();
    if (info.mHasWorldId)
        info.mWorldId = stream.getUuid();

    info.mHasWorldName = stream.getOptionalPresent();
    if (info.mHasWorldName)
        info.mWorldName = stream.getString();

    info.mCreatorId = stream.getString();

    info.mHasTargetId = stream.getOptionalPresent();
    if (info.mHasTargetId)
        info.mTargetId = stream.getUuid();

    info.mHasScenarioId = stream.getOptionalPresent();
    if (info.mHasScenarioId)
        info.mScenarioId = stream.getString();

    info.mHasServerId = stream.getOptionalPresent();
    if (info.mHasServerId)
        info.mServerId = stream.getString();
}

void TransferPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
