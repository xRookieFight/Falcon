#pragma once

#include "Command/Command.h"

class ServerNetworkHandler;

class DeopCommand : public Command {
public:
    explicit DeopCommand(ServerNetworkHandler &handler);

    bool execute(CommandSender &sender, const std::vector<std::string> &arguments) override;

private:
    ServerNetworkHandler &mHandler;
};
