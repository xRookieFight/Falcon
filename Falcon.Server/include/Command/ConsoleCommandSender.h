#pragma once

#include "Command/CommandSender.h"

class ConsoleCommandSender : public CommandSender {
public:
    const std::string &getSenderName() const override;

    bool isPlayer() const override { return false; }

    ServerPlayer *asPlayer() override { return nullptr; }

    void sendMessage(const std::string &message) override;

    void sendTranslation(const std::string &key, const std::vector<std::string> &parameters) override;

    CommandPermission getCommandPermission() const override { return CommandPermission::Internal; }
};
