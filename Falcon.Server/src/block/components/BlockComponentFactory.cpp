#include "block/components/BlockComponentFactory.h"

#include "block/components/BlockBehaviorRegistry.h"

const BlockBehavior &BlockComponentFactory::getBehavior(const std::string &identifier) {
    return BlockBehaviorRegistry::get(identifier);
}
