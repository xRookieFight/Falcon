#include "Command/TimeCommand.h"

#include "Network/ServerNetworkHandler.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>

TimeCommand::TimeCommand(ServerNetworkHandler &handler)
        : Command("time", "commands.time.description", "/time <set|add|query> ..."), mHandler(handler) {}

bool TimeCommand::parseInteger(const std::string &value, int64_t &out) {
    if (value.empty())
        return false;
    char *end = nullptr;
    errno = 0;
    const long long parsed = std::strtoll(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0' || errno == ERANGE)
        return false;
    out = (int64_t) parsed;
    return true;
}

bool TimeCommand::parseTime(const std::string &value, int64_t &out) {
    std::string lowered = value;
    for (char &character: lowered)
        character = (char) std::tolower((unsigned char) character);

    if (lowered == "day") {
        out = 1000;
        return true;
    }
    if (lowered == "noon") {
        out = 6000;
        return true;
    }
    if (lowered == "night") {
        out = 13000;
        return true;
    }
    if (lowered == "midnight") {
        out = 18000;
        return true;
    }
    return parseInteger(value, out);
}

std::vector<CommandOverloadData> TimeCommand::getOverloads() const {
    CommandParamData operation;
    operation.mName = "operation";
    operation.mHasEnumData = true;
    operation.mEnumData.mName = "TimeOperation";
    operation.mEnumData.mValues = {"set", "add", "query"};

    CommandParamData value;
    value.mName = "value";
    value.mOptional = true;
    value.mHasType = true;
    value.mType = CommandParamType::String;

    CommandOverloadData overload;
    overload.mParameters = {operation, value};
    return {overload};
}

bool TimeCommand::execute(CommandSender &sender, const std::vector<std::string> &arguments) {
    if (arguments.size() < 2) {
        sender.sendTranslation("commands.generic.usage", {getUsage()});
        return false;
    }

    std::string operation = arguments[0];
    for (char &character: operation)
        character = (char) std::tolower((unsigned char) character);

    if (operation == "set") {
        int64_t time = 0;
        if (!parseTime(arguments[1], time) || time < 0) {
            sender.sendTranslation("commands.time.invalid", {arguments[1]});
            return false;
        }
        mHandler.getLevel().setTime(time);
        mHandler.broadcastWorldTime();
        sender.sendTranslation("commands.time.set", {std::to_string(mHandler.getLevel().getDayTime())});
        return true;
    }

    if (operation == "add") {
        int64_t time = 0;
        if (!parseInteger(arguments[1], time)) {
            sender.sendTranslation("commands.time.invalid", {arguments[1]});
            return false;
        }
        mHandler.getLevel().addTime(time);
        mHandler.broadcastWorldTime();
        sender.sendTranslation("commands.time.added", {std::to_string(time)});
        return true;
    }

    if (operation == "query") {
        const std::string query = arguments[1];
        if (query == "daytime")
            sender.sendTranslation("commands.time.query.daytime", {std::to_string(mHandler.getLevel().getDayTime())});
        else if (query == "gametime")
            sender.sendTranslation("commands.time.query.gametime", {std::to_string(mHandler.getLevel().getTime())});
        else if (query == "day")
            sender.sendTranslation("commands.time.query.day", {std::to_string(mHandler.getLevel().getTime() / 24000)});
        else {
            sender.sendTranslation("commands.time.invalid", {query});
            return false;
        }
        return true;
    }

    sender.sendTranslation("commands.generic.usage", {getUsage()});
    return false;
}
