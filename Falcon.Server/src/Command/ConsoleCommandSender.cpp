#include "Command/ConsoleCommandSender.h"

#include "Core/Debug/BedrockLog.h"

const std::string &ConsoleCommandSender::getSenderName() const {
    static const std::string name = "Console";
    return name;
}

void ConsoleCommandSender::sendMessage(const std::string &message) {
    LOG_INFO(LogAreaID::Server, "%s", message.c_str());
}

void ConsoleCommandSender::sendTranslation(const std::string &key, const std::vector<std::string> &parameters) {
    std::string message = key;
    for (const std::string &parameter: parameters)
        message += " " + parameter;

    LOG_INFO(LogAreaID::Server, "%s", message.c_str());
}
