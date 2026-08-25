#pragma once

#include "core/utility/BinaryStream.h"
#include "core/utility/ReadOnlyBinaryStream.h"

#include <vector>

class EntityFlagCodec {
public:
    static void writeFlags(BinaryStream &stream, const std::vector<bool> &flags);

    static std::vector<bool> readFlags(ReadOnlyBinaryStream &stream);
};
