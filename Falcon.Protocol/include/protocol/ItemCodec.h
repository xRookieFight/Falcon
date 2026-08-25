#pragma once

#include "core/utility/BinaryStream.h"
#include "core/utility/ReadOnlyBinaryStream.h"
#include "protocol/PacketCodecContext.h"
#include "protocol/types/ItemStack.h"

class ItemCodec {
public:
    static void writeItemInstance(BinaryStream &stream, const PacketCodecContext &context, const ItemStack &item);

    static ItemStack readItemInstance(ReadOnlyBinaryStream &stream, const PacketCodecContext &context);

    static void writeNetworkItemStackDescriptor(BinaryStream &stream, const PacketCodecContext &context, const ItemStack &item);

    static ItemStack readNetworkItemStackDescriptor(ReadOnlyBinaryStream &stream, const PacketCodecContext &context);
};
