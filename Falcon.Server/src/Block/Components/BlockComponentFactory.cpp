#include "Block/Components/BlockComponentFactory.h"

#include "Block/Components/BlockBehaviorRegistry.h"

const BlockBehavior &BlockComponentFactory::getBehavior(const std::string &identifier) {
    return BlockBehaviorRegistry::get(identifier);
}
