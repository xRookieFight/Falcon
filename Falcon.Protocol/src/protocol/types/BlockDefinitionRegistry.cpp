#include "protocol/types/BlockDefinitionRegistry.h"

void BlockDefinitionRegistry::registerDefinition(std::shared_ptr<BlockDefinition> definition) {
    int runtimeId = definition->getRuntimeId();
    const std::string &identifier = definition->getIdentifier();
    mByIdentifier.try_emplace(identifier, definition);
    mByRuntimeId[runtimeId] = std::move(definition);
}

std::shared_ptr<BlockDefinition> BlockDefinitionRegistry::getDefinition(int runtimeId) const {
    auto it = mByRuntimeId.find(runtimeId);
    return it == mByRuntimeId.end() ? nullptr : it->second;
}

std::shared_ptr<BlockDefinition> BlockDefinitionRegistry::getDefinition(const std::string &identifier) const {
    auto it = mByIdentifier.find(identifier);
    return it == mByIdentifier.end() ? nullptr : it->second;
}

bool BlockDefinitionRegistry::isRegistered(const std::shared_ptr<BlockDefinition> &definition) const {
    if (!definition) return false;
    auto it = mByRuntimeId.find(definition->getRuntimeId());
    return it != mByRuntimeId.end() && it->second == definition;
}
