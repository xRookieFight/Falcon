#include "block/components/BlockBehaviorRegistry.h"

#include "block/components/BlockBehavior.h"
#include "block/blocks/SlimeBlockBehavior.h"

#include <unordered_map>

namespace {
    using BehaviorMap = std::unordered_map<std::string, const BlockBehavior *>;

    BehaviorMap &behaviorMap() {
        static BehaviorMap map;
        return map;
    }

    const BlockBehavior &defaultBehavior() {
        static const BlockBehavior behavior;
        return behavior;
    }

    const SlimeBlockBehavior &slimeBehavior() {
        static const SlimeBlockBehavior behavior;
        return behavior;
    }

    void registerVanillaBehaviors() {
        static const bool registered = [] {
            BlockBehaviorRegistry::registerBehavior("minecraft:slime", slimeBehavior());
            return true;
        }();
        (void) registered;
    }
}

const BlockBehavior &BlockBehaviorRegistry::get(const std::string &identifier) {
    registerVanillaBehaviors();

    const auto it = behaviorMap().find(identifier);
    return it == behaviorMap().end() ? defaultBehavior() : *it->second;
}

void BlockBehaviorRegistry::registerBehavior(const std::string &identifier, const BlockBehavior &behavior) {
    behaviorMap()[identifier] = &behavior;
}
