#pragma once

#include "Command/Command.h"

class ServerNetworkHandler;

class OpCommand : public Command {
public:
    explicit OpCommand(ServerNetworkHandler &handler);

    bool execute(CommandSender &sender, const std::vector<std::string> &arguments) override;

    std::vector<CommandOverloadData> getOverloads() const override;

private:
    ServerNetworkHandler &mHandler;
};
