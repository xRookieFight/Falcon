#include "protocol/types/ItemDefinition.h"

ItemDefinition::ItemDefinition(std::string identifier, int runtimeId, bool componentBased, Tag componentData)
        : mIdentifier(std::move(identifier)), mRuntimeId(runtimeId), mComponentBased(componentBased),
          mComponentData(std::move(componentData)) {}
