#include "Command/ClearCommand.h"

#include "Actor/ServerPlayer.h"
#include "Network/Handler/ServerNetworkHandler.h"

ClearCommand::ClearCommand(ServerNetworkHandler &handler)
        : Command("clear", "commands.clear.description", "/clear [player]"), mHandler(handler) {}

CommandPermission ClearCommand::getRequiredPermission() const {
    return CommandPermission::Any;
}

std::vector<CommandOverloadData> ClearCommand::getOverloads() const {
    CommandParamData playerParameter = makePlayerParameter("player", mHandler.getPlayerNames());
    playerParameter.mOptional = true;

    CommandOverloadData overload;
    overload.mParameters.push_back(playerParameter);

    return {overload};
}

bool ClearCommand::execute(CommandOrigin &sender, const std::vector<std::string> &arguments) {
    std::vector<ServerPlayer *> targets;

    if (arguments.empty()) {
        ServerPlayer *self = sender.asPlayer();
        if (self == nullptr) {
            sender.sendTranslation("commands.generic.playerOnly", {});
            return false;
        }

        targets.push_back(self);
    } else {
        targets = mHandler.resolveTargets(sender, arguments[0]);
    }

    if (targets.empty()) {
        sender.sendTranslation("commands.generic.selector.empty", {});
        return false;
    }

    for (ServerPlayer *target: targets) {
        target->getInventory().clear();
        target->getInventoryManager().syncAll();
        sender.sendTranslation("commands.clear.success", {target->getName()});
    }

    return true;
}
