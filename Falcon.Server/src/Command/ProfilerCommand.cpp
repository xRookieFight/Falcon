#include "command/ProfilerCommand.h"

#include "network/handler/ServerNetworkHandler.h"

#include <cctype>
#include <string>

ProfilerCommand::ProfilerCommand(ServerNetworkHandler &handler)
        : Command("profiler", "commands.profiler.description", "/profiler <on|off|status>"), mHandler(handler) {}

std::vector<CommandOverloadData> ProfilerCommand::getOverloads() const {
    CommandParamData mode;
    mode.mName = "mode";
    mode.mOptional = false;
    mode.mHasEnumData = true;
    mode.mEnumData.mName = "ProfilerMode";
    mode.mEnumData.mValues = {"on", "off", "status"};

    CommandOverloadData overload;
    overload.mParameters = {mode};
    return {overload};
}

bool ProfilerCommand::execute(CommandOrigin &sender, const std::vector<std::string> &arguments) {
    if (arguments.empty()) {
        sender.sendTranslation("commands.generic.usage", {getUsage()});
        return false;
    }

    std::string mode = arguments[0];
    for (char &character: mode)
        character = (char) std::tolower((unsigned char) character);

    Profiler &profiler = mHandler.getProfiler();

    if (mode == "status") {
        if (!profiler.isActive()) {
            sender.sendMessage("§7The profiler is §coff§7.");
            return true;
        }

        sender.sendMessage("§7The profiler is §aon§7, §a" + std::to_string(profiler.getSampleCount()) +
                           "§7 tick(s) recorded since tick §a" + std::to_string(profiler.getStartTick()) + "§7.");
        return true;
    }

    if (mode == "on") {
        if (profiler.isActive()) {
            sender.sendMessage("§cThe profiler is already running.");
            return false;
        }

        profiler.start(mHandler.getCurrentTick());
        sender.sendMessage("§aProfiler started.§7 Run §f/profiler off§7 to write the report.");
        return true;
    }

    if (mode != "off") {
        sender.sendTranslation("commands.generic.usage", {getUsage()});
        return false;
    }

    if (!profiler.isActive()) {
        sender.sendMessage("§cThe profiler is not running.");
        return false;
    }

    std::string path;
    std::string error;

    if (!profiler.stop("profiler", path, error)) {
        sender.sendMessage("§cProfiler stopped but no report was written: " + error);
        return false;
    }

    sender.sendMessage("§aProfiler stopped.§7 Report written to §f" + path);
    return true;
}
