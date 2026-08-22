#include "Command/OpCommand.h"

#include "Network/ServerNetworkHandler.h"

OpCommand::OpCommand(ServerNetworkHandler &handler)
        : Command("op", "Grants operator status to a player", "/op <player>"), mHandler(handler) {}

bool OpCommand::execute(CommandSender &sender, const std::vector<std::string> &arguments) {
    if (arguments.empty()) {
        sender.sendMessage("Usage: " + getUsage());
        return false;
    }

    mHandler.getOpList().addOp(arguments[0]);

    ServerPlayer *target = mHandler.getPlayerByName(arguments[0]);
    if (target != nullptr)
        mHandler.setPlayerOp(*target, true);

    sender.sendMessage("Opped " + arguments[0]);
    return true;
}

std::vector<CommandOverloadData> OpCommand::getOverloads() const {
    CommandOverloadData overload;
    overload.mParameters.push_back(makePlayerParameter("player", mHandler.getPlayerNames()));
    return {overload};
}
