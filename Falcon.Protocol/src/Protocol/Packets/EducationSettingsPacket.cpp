#include "Protocol/Packets/EducationSettingsPacket.h"

#include "Protocol/NetworkPacketHandler.h"

EducationSettingsPacket::EducationSettingsPacket() = default;

void EducationSettingsPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putString(mCodeBuilderUri);
    stream.putString(mCodeBuilderTitle);
    stream.putBool(mCanResizeCodeBuilder);
    stream.putBool(mDisableLegacyTitle);
    stream.putString(mPostProcessFilter);
    stream.putString(mScreenshotBorderPath);

    stream.putOptionalPresent(mHasEntityCapabilities);
    if (mHasEntityCapabilities)
        stream.putBool(mEntityCapabilities);

    stream.putOptionalPresent(mHasOverrideUri);
    if (mHasOverrideUri)
        stream.putString(mOverrideUri);

    stream.putBool(mQuizAttached);

    stream.putOptionalPresent(mHasExternalLinkSettings);
    if (mHasExternalLinkSettings)
        stream.putBool(mExternalLinkSettings);
}

void EducationSettingsPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mCodeBuilderUri = stream.getString();
    mCodeBuilderTitle = stream.getString();
    mCanResizeCodeBuilder = stream.getBool();
    mDisableLegacyTitle = stream.getBool();
    mPostProcessFilter = stream.getString();
    mScreenshotBorderPath = stream.getString();

    mHasEntityCapabilities = stream.getOptionalPresent();
    if (mHasEntityCapabilities)
        mEntityCapabilities = stream.getBool();

    mHasOverrideUri = stream.getOptionalPresent();
    if (mHasOverrideUri)
        mOverrideUri = stream.getString();

    mQuizAttached = stream.getBool();

    mHasExternalLinkSettings = stream.getOptionalPresent();
    if (mHasExternalLinkSettings)
        mExternalLinkSettings = stream.getBool();
}

void EducationSettingsPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
