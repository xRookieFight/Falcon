#pragma once

#include "core/nbt/Tag.h"
#include "protocol/PacketCodecContext.h"
#include "protocol/types/ItemStack.h"

class ItemStackNbt {
public:
    static Tag write(const ItemStack &item, int slot);

    static Tag write(const ItemStack &item);

    static ItemStack read(const Tag &data, const PacketCodecContext &context);

    static int readSlot(const Tag &data);
};
