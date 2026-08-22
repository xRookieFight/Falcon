#include "Command/KillCommand.h"

#include "Entity/ServerPlayer.h"
#include "Network/ServerNetworkHandler.h"

KillCommand::KillCommand(ServerNetworkHandler &handler)
        : Command("kill", "Kills a player", "/kill [player]"), mHandler(handler) {}

std::vector<CommandOverloadData> KillCommand::getOverloads() const {
    CommandParamData playerParameter = makePlayerParameter("player", mHandler.getPlayerNames());
    playerParameter.mOptional = true;

    CommandOverloadData overload;
    overload.mParameters.push_back(playerParameter);

    return {overload};
}

bool KillCommand::execute(CommandSender &sender, const std::vector<std::string> &arguments) {
    std::vector<ServerPlayer *> targets;

    if (arguments.empty()) {
        ServerPlayer *self = sender.asPlayer();
        if (self == nullptr) {
            sender.sendMessage("§cThis command can only be run by a player");
            return false;
        }

        targets.push_back(self);
    } else {
        targets = mHandler.resolveTargets(sender, arguments[0]);
    }

    if (targets.empty()) {
        sender.sendMessage("§cNo targets matched selector");
        return false;
    }

    for (ServerPlayer *target: targets) {
        if (target->isDead())
            continue;

        mHandler.killPlayer(*target, "§e" + target->getName() + " died");
        sender.sendMessage("Killed " + target->getName());
    }

    return true;
}
