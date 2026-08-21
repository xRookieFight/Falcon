#include "Protocol/Types/CommandOriginData.h"

namespace {
    bool carriesPlayerId(CommandOriginType origin) {
        return origin == CommandOriginType::DevConsole || origin == CommandOriginType::Test;
    }
}

void CommandOriginData::write(BinaryStream &stream) const {
    stream.putUnsignedVarInt((uint32_t) mOrigin);
    stream.putUuid(mUuid);
    stream.putString(mRequestId);

    if (carriesPlayerId(mOrigin))
        stream.putVarLong(mPlayerId);
}

void CommandOriginData::read(ReadOnlyBinaryStream &stream) {
    mOrigin = (CommandOriginType) stream.getUnsignedVarInt();
    mUuid = stream.getUuid();
    mRequestId = stream.getString();
    mPlayerId = carriesPlayerId(mOrigin) ? stream.getVarLong() : -1;
}
