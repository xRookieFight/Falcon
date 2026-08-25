#include "protocol/packets/PlayerListPacket.h"

#include "protocol/NetworkPacketHandler.h"
#include "protocol/SkinCodec.h"

namespace {

    void writeEntryBase(BinaryStream &stream, const PlayerListPacket::Entry &entry) {
        stream.putUuid(entry.mUuid);
        stream.putVarLong(entry.mActorId);
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
        entry.mActorId = stream.getVarLong();
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
    stream.putUnsignedVarInt((uint32_t) mEntries.size());

    for (const Entry &entry: mEntries) {
        stream.putUnsignedVarInt(entry.mAction == Action::Add ? 1 : 0);
        stream.putByte((uint8_t) entry.mAction);

        if (entry.mAction == Action::Add)
            writeEntryBase(stream, entry);
        else
            stream.putUuid(entry.mUuid);
    }
}

void PlayerListPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    const uint32_t length = stream.getUnsignedVarInt();

    mEntries.clear();
    mEntries.reserve(length);

    for (uint32_t i = 0; i < length; i++) {
        const Action action = stream.getUnsignedVarInt() == 1 ? Action::Add : Action::Remove;
        stream.getByte();

        Entry entry = action == Action::Add ? readEntryBase(stream) : Entry(stream.getUuid());
        entry.mAction = action;
        mEntries.push_back(entry);
    }
}

void PlayerListPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
