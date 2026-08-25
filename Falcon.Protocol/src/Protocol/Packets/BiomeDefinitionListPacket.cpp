#include "protocol/packets/BiomeDefinitionListPacket.h"

#include "protocol/NetworkPacketHandler.h"

namespace {

    void writeDefinition(BinaryStream &stream, const BiomeDefinitionData &biome) {
        stream.putLShort((uint16_t) 0xFFFF);

        stream.putLFloat(biome.mTemperature);
        stream.putLFloat(biome.mDownfall);
        stream.putLFloat(biome.mFoliageSnow);
        stream.putLFloat(biome.mDepth);
        stream.putLFloat(biome.mScale);
        stream.putLInt((uint32_t) biome.mMapWaterColorArgb);
        stream.putBool(biome.mRain);

        stream.putBool(false);
        stream.putBool(false);
    }

}

BiomeDefinitionListPacket::BiomeDefinitionListPacket() = default;

void BiomeDefinitionListPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putArrayLength((uint32_t) mBiomes.size());
    for (uint32_t i = 0; i < (uint32_t) mBiomes.size(); i++) {
        stream.putLShort((uint16_t) i);
        writeDefinition(stream, mBiomes[i]);
    }

    stream.putArrayLength((uint32_t) mBiomes.size());
    for (const BiomeDefinitionData &biome: mBiomes)
        stream.putString(biome.mName);
}

void BiomeDefinitionListPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mBiomes.clear();

    const uint32_t biomeCount = stream.getArrayLength();
    std::vector<BiomeDefinitionData> ordered(biomeCount);

    for (uint32_t i = 0; i < biomeCount; i++) {
        const uint16_t stringIndex = stream.getLShort();

        BiomeDefinitionData biome;
        stream.getLShort();
        biome.mTemperature = stream.getLFloat();
        biome.mDownfall = stream.getLFloat();
        biome.mFoliageSnow = stream.getLFloat();
        biome.mDepth = stream.getLFloat();
        biome.mScale = stream.getLFloat();
        biome.mMapWaterColorArgb = (int32_t) stream.getLInt();
        biome.mRain = stream.getBool();

        stream.getBool();
        stream.getBool();

        if (stringIndex < ordered.size())
            ordered[stringIndex] = biome;
    }

    const uint32_t stringCount = stream.getArrayLength();
    for (uint32_t i = 0; i < stringCount; i++) {
        const std::string name = stream.getString();
        if (i < ordered.size())
            ordered[i].mName = name;
    }

    mBiomes = std::move(ordered);
}

void BiomeDefinitionListPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
