#pragma once

#include "Core/Utility/BinaryStream.h"
#include "Core/Utility/ReadOnlyBinaryStream.h"
#include "Protocol/Types/SerializedSkin.h"

class SkinCodec {
public:
    static void writeSkin(BinaryStream &stream, const SerializedSkin &skin);

    static SerializedSkin readSkin(ReadOnlyBinaryStream &stream);

private:
    static void writeImage(BinaryStream &stream, const SkinImageData &image);

    static SkinImageData readImage(ReadOnlyBinaryStream &stream);

    static void writeAnimation(BinaryStream &stream, const SkinAnimationData &animation);

    static SkinAnimationData readAnimation(ReadOnlyBinaryStream &stream);
};
