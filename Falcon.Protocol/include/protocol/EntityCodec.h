#pragma once

#include "core/utility/BinaryStream.h"
#include "core/utility/ReadOnlyBinaryStream.h"
#include "protocol/types/AttributeData.h"
#include "protocol/types/EntityDataMap.h"
#include "protocol/types/EntityLinkData.h"
#include "protocol/types/EntityProperties.h"
#include "protocol/types/PlayerAbilityData.h"

class EntityCodec {
public:
    static void writeAttribute(BinaryStream &stream, const AttributeData &attribute);

    static AttributeData readAttribute(ReadOnlyBinaryStream &stream);

    static void writeEntityData(BinaryStream &stream, const EntityDataMap &entityData);

    static EntityDataMap readEntityData(ReadOnlyBinaryStream &stream);

    static void writeEntityProperties(BinaryStream &stream, const EntityProperties &properties);

    static EntityProperties readEntityProperties(ReadOnlyBinaryStream &stream);

    static void writeEntityLink(BinaryStream &stream, const EntityLinkData &link);

    static EntityLinkData readEntityLink(ReadOnlyBinaryStream &stream);

    static void writePlayerAbilities(BinaryStream &stream, const PlayerAbilityData &abilities);

    static PlayerAbilityData readPlayerAbilities(ReadOnlyBinaryStream &stream);
};
