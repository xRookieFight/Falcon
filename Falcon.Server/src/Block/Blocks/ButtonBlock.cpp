#include "Block/Blocks/ButtonBlock.h"

#include "Block/Systems/RedstoneSystem.h"

namespace {
    const char *BUTTON_SUFFIX = "_button";
    const size_t BUTTON_SUFFIX_LENGTH = 7;
}

bool ButtonBlock::matches(const std::string &identifier)
{
    if (identifier.size() < BUTTON_SUFFIX_LENGTH)
        return false;

    return identifier.compare(identifier.size() - BUTTON_SUFFIX_LENGTH, BUTTON_SUFFIX_LENGTH,
                              BUTTON_SUFFIX) == 0;
}

bool ButtonBlock::onInteract(ServerNetworkHandler &owner, ServerPlayer &player, const Vector3i &position,
                             const BlockState &state) const
{
    (void) player;

    RedstoneSystem::onButtonActivated(owner, position, state);
    return true;
}
