#pragma once

#include "core/utility/BinaryStream.h"
#include "core/utility/ReadOnlyBinaryStream.h"
#include "protocol/PacketCodecContext.h"
#include "protocol/types/FullContainerName.h"
#include "protocol/types/InventoryActionData.h"
#include "protocol/types/InventorySource.h"
#include "protocol/types/ItemStackRequest.h"
#include "protocol/types/ItemStackRequestSlotData.h"
#include "protocol/types/ItemUseTransaction.h"
#include "protocol/types/PlayerBlockActionData.h"

class InventoryCodec {
public:
    static void writeFullContainerName(BinaryStream &stream, const FullContainerName &name);

    static FullContainerName readFullContainerName(ReadOnlyBinaryStream &stream);

    static void writeInventorySource(BinaryStream &stream, const InventorySource &source);

    static InventorySource readInventorySource(ReadOnlyBinaryStream &stream);

    static void writeInventoryActions(BinaryStream &stream, const PacketCodecContext &context,
                                       const std::vector<InventoryActionData> &actions);

    static void readInventoryActions(ReadOnlyBinaryStream &stream, const PacketCodecContext &context,
                                      std::vector<InventoryActionData> &actions);

    static void writeStackRequestSlotInfo(BinaryStream &stream, const ItemStackRequestSlotData &data);

    static ItemStackRequestSlotData readStackRequestSlotInfo(ReadOnlyBinaryStream &stream);

    static void writeItemStackRequest(BinaryStream &stream, const PacketCodecContext &context, const ItemStackRequest &request);

    static ItemStackRequest readItemStackRequest(ReadOnlyBinaryStream &stream, const PacketCodecContext &context);

    static void writeItemUseTransaction(BinaryStream &stream, const PacketCodecContext &context, const ItemUseTransaction &transaction);

    static ItemUseTransaction readItemUseTransaction(ReadOnlyBinaryStream &stream, const PacketCodecContext &context);

    static void writePlayerBlockActionData(BinaryStream &stream, const PlayerBlockActionData &data);

    static PlayerBlockActionData readPlayerBlockActionData(ReadOnlyBinaryStream &stream);
};
