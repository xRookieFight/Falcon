#pragma once

#include "core/utility/BinaryStream.h"
#include "core/utility/ReadOnlyBinaryStream.h"
#include "protocol/types/SerializedSkin.h"

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
