#include "Protocol/Packets/PlayerListPacket.h"

#include "Protocol/NetworkPacketHandler.h"
#include "Protocol/SkinCodec.h"

namespace {

    void writeEntryBase(BinaryStream &stream, const PlayerListPacket::Entry &entry) {
        stream.putUuid(entry.mUuid);
        stream.putVarLong(entry.mEntityId);
        stream.putString(entry.mName);
        stream.putString(entry.mXuid);
        stream.putString(entry.mPlatformChatId);
        stream.putLInt((uint32_t) entry.mBuildPlatform);
        SkinCodec::writeSkin(stream, entry.mSkin);
        stream.putBool(entry.mTeacher);
        stream.putBool(entry.mHost);
        stream.putBool(entry.mSubClient);
        stream.putLInt((uint32_t) entry.mColorArgb);
    }

    PlayerListPacket::Entry readEntryBase(ReadOnlyBinaryStream &stream) {
        PlayerListPacket::Entry entry(stream.getUuid());
        entry.mEntityId = stream.getVarLong();
        entry.mName = stream.getString();
        entry.mXuid = stream.getString();
        entry.mPlatformChatId = stream.getString();
        entry.mBuildPlatform = (int32_t) stream.getLInt();
        entry.mSkin = SkinCodec::readSkin(stream);
        entry.mTeacher = stream.getBool();
        entry.mHost = stream.getBool();
        entry.mSubClient = stream.getBool();
        entry.mColorArgb = (int32_t) stream.getLInt();
        return entry;
    }

}

PlayerListPacket::PlayerListPacket() = default;

void PlayerListPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putByte((uint8_t) mAction);
    stream.putUnsignedVarInt((uint32_t) mEntries.size());

    if (mAction == Action::Add) {
        for (const Entry &entry: mEntries)
            writeEntryBase(stream, entry);

        for (const Entry &entry: mEntries)
            stream.putBool(entry.mTrustedSkin);
    } else {
        for (const Entry &entry: mEntries)
            stream.putUuid(entry.mUuid);
    }
}

void PlayerListPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mAction = (Action) stream.getByte();
    const uint32_t length = stream.getUnsignedVarInt();

    mEntries.clear();
    mEntries.reserve(length);

    if (mAction == Action::Add) {
        for (uint32_t i = 0; i < length; i++)
            mEntries.push_back(readEntryBase(stream));

        for (uint32_t i = 0; i < length && !stream.feof(); i++)
            mEntries[i].mTrustedSkin = stream.getBool();
    } else {
        for (uint32_t i = 0; i < length; i++)
            mEntries.emplace_back(stream.getUuid());
    }
}

void PlayerListPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
