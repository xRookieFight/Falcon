#pragma once

#include "Core/Math/Vector3f.h"
#include "Core/Math/Vector3i.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

class ServerNetworkHandler;
class ServerPlayer;
class ItemStack;

class ItemBehavior {
public:
    virtual ~ItemBehavior() = default;

    virtual bool onUse(ServerNetworkHandler &owner, ServerPlayer &player, const ItemStack &item) {
        (void) owner;
        (void) player;
        (void) item;
        return false;
    }

    virtual bool onUseOnBlock(ServerNetworkHandler &owner, ServerPlayer &player, const ItemStack &item,
                              const Vector3i &blockPosition, int32_t face, const Vector3f &clickPosition) {
        (void) owner;
        (void) player;
        (void) item;
        (void) blockPosition;
        (void) face;
        (void) clickPosition;
        return false;
    }
};

class ItemBehaviorRegistry {
public:
    static ItemBehaviorRegistry &getInstance();

    void registerBehavior(const std::string &identifier, std::unique_ptr<ItemBehavior> behavior);

    ItemBehavior *find(const std::string &identifier) const;

    void registerVanilla();

private:
    std::unordered_map<std::string, std::unique_ptr<ItemBehavior>> mBehaviors;
};
