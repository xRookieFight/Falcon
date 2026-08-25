#pragma once

#include "core/utility/BinaryStream.h"
#include "core/utility/ReadOnlyBinaryStream.h"
#include "protocol/types/CameraTypes.h"

class CameraCodec {
public:
    static void writeEase(BinaryStream &stream, const CameraEaseData &ease);

    static CameraEaseData readEase(ReadOnlyBinaryStream &stream);

    static void writeSetInstruction(BinaryStream &stream, const CameraSetInstruction &instruction);

    static CameraSetInstruction readSetInstruction(ReadOnlyBinaryStream &stream);

    static void writeFadeInstruction(BinaryStream &stream, const CameraFadeInstruction &instruction);

    static CameraFadeInstruction readFadeInstruction(ReadOnlyBinaryStream &stream);

    static void writeSplineInstruction(BinaryStream &stream, const CameraSplineInstruction &instruction);

    static CameraSplineInstruction readSplineInstruction(ReadOnlyBinaryStream &stream);

    static void writeAimAssistPreset(BinaryStream &stream, const CameraAimAssistPreset &preset);

    static CameraAimAssistPreset readAimAssistPreset(ReadOnlyBinaryStream &stream);

    static void writePreset(BinaryStream &stream, const CameraPreset &preset);

    static CameraPreset readPreset(ReadOnlyBinaryStream &stream);
};
