#include "Protocol/Packets/AddEntityPacket.h"

#include "Protocol/EntityCodec.h"
#include "Protocol/NetworkPacketHandler.h"

AddEntityPacket::AddEntityPacket() = default;

void AddEntityPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putVarLong(mUniqueEntityId);
    stream.putUnsignedVarLong((uint64_t) mRuntimeEntityId);
    stream.putString(mIdentifier);
    stream.putVector3f(mPosition);
    stream.putVector3f(mMotion);
    stream.putVector2f(mRotation);
    stream.putLFloat(mHeadRotation);
    stream.putLFloat(mBodyRotation);

    stream.putArrayLength((uint32_t) mAttributes.size());
    for (const AttributeData &attribute: mAttributes) {
        EntityCodec::writeAttribute(stream, attribute);
    }

    EntityCodec::writeEntityData(stream, mMetadata);
    EntityCodec::writeEntityProperties(stream, mProperties);

    stream.putArrayLength((uint32_t) mEntityLinks.size());
    for (const EntityLinkData &link: mEntityLinks) {
        EntityCodec::writeEntityLink(stream, link);
    }
}

void AddEntityPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mUniqueEntityId = stream.getVarLong();
    mRuntimeEntityId = (int64_t) stream.getUnsignedVarLong();
    mIdentifier = stream.getString();
    mPosition = stream.getVector3f();
    mMotion = stream.getVector3f();
    mRotation = stream.getVector2f();
    mHeadRotation = stream.getLFloat();
    mBodyRotation = stream.getLFloat();

    uint32_t attributeCount = stream.getArrayLength();
    mAttributes.reserve(attributeCount);
    for (uint32_t i = 0; i < attributeCount; i++) {
        mAttributes.push_back(EntityCodec::readAttribute(stream));
    }

    mMetadata = EntityCodec::readEntityData(stream);
    mProperties = EntityCodec::readEntityProperties(stream);

    uint32_t linkCount = stream.getArrayLength();
    mEntityLinks.reserve(linkCount);
    for (uint32_t i = 0; i < linkCount; i++) {
        mEntityLinks.push_back(EntityCodec::readEntityLink(stream));
    }
}

void AddEntityPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
