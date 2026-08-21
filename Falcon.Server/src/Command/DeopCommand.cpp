#include "Command/DeopCommand.h"

#include "Network/ServerNetworkHandler.h"

DeopCommand::DeopCommand(ServerNetworkHandler &handler)
        : Command("deop", "Revokes operator status from a player", "/deop <player>"), mHandler(handler) {}

bool DeopCommand::execute(CommandSender &sender, const std::vector<std::string> &arguments) {
    if (arguments.empty()) {
        sender.sendMessage("Usage: " + getUsage());
        return false;
    }

    mHandler.getOpList().removeOp(arguments[0]);

    ServerPlayer *target = mHandler.getPlayerByName(arguments[0]);
    if (target != nullptr)
        mHandler.setPlayerOp(*target, false);

    sender.sendMessage("Deopped " + arguments[0]);
    return true;
}
