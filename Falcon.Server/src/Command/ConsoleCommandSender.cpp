#include "Command/ConsoleCommandSender.h"

#include "Core/Debug/BedrockLog.h"

const std::string &ConsoleCommandSender::getSenderName() const {
    static const std::string name = "Console";
    return name;
}

void ConsoleCommandSender::sendMessage(const std::string &message) {
    LOG_INFO(LogAreaID::Server, "%s", message.c_str());
}
