#include "Command/CommandMap.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
    std::string toLowerCase(const std::string &value) {
        std::string lowered = value;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                       [](unsigned char character) { return (char) std::tolower(character); });
        return lowered;
    }
}

void CommandMap::registerCommand(std::shared_ptr<Command> command) {
    if (command == nullptr)
        return;

    mByName[toLowerCase(command->getName())] = command.get();
    for (const std::string &alias: command->getAliases())
        mByName[toLowerCase(alias)] = command.get();

    mCommands.push_back(std::move(command));
}

Command *CommandMap::getCommand(const std::string &name) const {
    auto it = mByName.find(toLowerCase(name));
    return it == mByName.end() ? nullptr : it->second;
}

std::vector<std::string> CommandMap::_tokenize(const std::string &commandLine) {
    std::vector<std::string> tokens;
    std::istringstream stream(commandLine);
    std::string token;

    while (stream >> token)
        tokens.push_back(token);

    return tokens;
}

bool CommandMap::dispatch(CommandOrigin &sender, const std::string &commandLine) {
    std::string line = commandLine;
    if (!line.empty() && line[0] == '/')
        line.erase(0, 1);

    std::vector<std::string> tokens = _tokenize(line);
    if (tokens.empty())
        return false;

    Command *command = getCommand(tokens[0]);
    if (command == nullptr) {
        sender.sendTranslation("commands.generic.unknown", {tokens[0]});
        return false;
    }

    if ((int) sender.getCommandPermission() < (int) command->getRequiredPermission()) {
        sender.sendTranslation("commands.generic.permission", {});
        return false;
    }

    const std::vector<std::string> arguments(tokens.begin() + 1, tokens.end());
    return command->execute(sender, arguments);
}

std::vector<Command *> CommandMap::getCommands() const {
    std::vector<Command *> commands;
    commands.reserve(mCommands.size());

    for (const std::shared_ptr<Command> &command: mCommands)
        commands.push_back(command.get());

    return commands;
}
