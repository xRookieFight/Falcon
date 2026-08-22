#pragma once

#include "Inventory/PlayerInventory.h"
#include "Protocol/Packets/CreativeContentPacket.h"
#include "Protocol/Packets/ItemStackResponsePacket.h"
#include "Protocol/Types/ItemStackRequest.h"

#include <vector>

class ItemStackRequestHandler {
public:
    static const int RESULT_OK = 0;
    static const int RESULT_ERROR = 1;

    static ItemStackResponseEntry execute(PlayerInventory &inventory, const ItemStackRequest &request,
                                          const std::vector<CreativeItemData> &creativeItems);
};
