#include "command/StatusCommand.h"

#include "network/handler/ServerNetworkHandler.h"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <thread>

StatusCommand::StatusCommand(ServerNetworkHandler &handler)
        : Command("status", "commands.status.description", "/status [simple|full|tps]"), mHandler(handler) {}

std::vector<CommandOverloadData> StatusCommand::getOverloads() const {
    CommandParamData mode;
    mode.mName = "mode";
    mode.mOptional = true;
    mode.mHasEnumData = true;
    mode.mEnumData.mName = "StatusMode";
    mode.mEnumData.mValues = {"simple", "full", "tps"};

    CommandOverloadData overload;
    overload.mParameters = {mode};
    return {overload};
}

std::string StatusCommand::formatUptime(int64_t seconds) {
    const int64_t days = seconds / 86400;
    seconds %= 86400;
    const int64_t hours = seconds / 3600;
    seconds %= 3600;
    const int64_t minutes = seconds / 60;
    seconds %= 60;

    std::ostringstream stream;
    stream << days << "d " << hours << "h " << minutes << "m " << seconds << "s";
    return stream.str();
}

bool StatusCommand::execute(CommandOrigin &sender, const std::vector<std::string> &arguments) {
    std::string mode = arguments.empty() ? "full" : arguments[0];
    for (char &character: mode)
        character = (char) std::tolower((unsigned char) character);

    if (mode != "simple" && mode != "full" && mode != "tps") {
        sender.sendTranslation("commands.generic.usage", {getUsage()});
        return false;
    }

    std::ostringstream tps;
    tps << std::fixed << std::setprecision(2) << mHandler.getTicksPerSecond();

    std::ostringstream mspt;
    mspt << std::fixed << std::setprecision(2) << mHandler.getMillisecondsPerTick()
         << "§7 (peak §a" << std::fixed << std::setprecision(2) << mHandler.getPeakMillisecondsPerTick() << "§7)";

    if (mode == "tps") {
        sender.sendMessage("§6Current TPS: §a" + tps.str());
        sender.sendMessage("§6MSPT: §a" + mspt.str());
        sender.sendMessage("§6Tick: §a" + std::to_string(mHandler.getCurrentTick()));
        return true;
    }

    size_t online = 0;
    for (const auto &entry: mHandler.getPlayers()) {
        if (entry.second.isSpawned())
            ++online;
    }

    sender.sendMessage("§2---- §fServer status §2----");
    sender.sendMessage("§6Uptime: §a" + formatUptime(mHandler.getUptimeSeconds()));
    sender.sendMessage("§6Current TPS: §a" + tps.str());
    sender.sendMessage("§6MSPT: §a" + mspt.str());
    sender.sendMessage("§6Players: §a" + std::to_string(online) + "§7/§a" + std::to_string(mHandler.getMaxPlayers()));
    sender.sendMessage("§6World: §a" + mHandler.getLevel().getName() + "§7, time §a" +
                       std::to_string(mHandler.getLevel().getTime()) + "§7, day §a" +
                       std::to_string(mHandler.getLevel().getDayTime()));
    sender.sendMessage("§6Chunks: §a" + std::to_string(mHandler.getLevel().getLoadedChunkCount()));
    sender.sendMessage("§6Entities: §a" + std::to_string(online + mHandler.getItemEntities().size()));

    if (mode == "full") {
        sender.sendMessage("§6Connections: §a" + std::to_string(mHandler.getNetworkHandler().getConnectionCount()));
        sender.sendMessage("§6View distance: §a" + std::to_string(mHandler.getProperties().getViewDistance()));
        sender.sendMessage("§6Item entities: §a" + std::to_string(mHandler.getItemEntities().size()));
        sender.sendMessage("§6Threads: §a" + std::to_string(std::thread::hardware_concurrency()));

        const ChunkWorker *worker = mHandler.getLevel().getChunkWorker();
        if (worker != nullptr) {
            sender.sendMessage("§6Chunk threads: §a" + std::to_string(worker->getThreadCount()) +
                               "§7, pending §a" + std::to_string(worker->getPendingTaskCount()));
            sender.sendMessage("§6Chunks generated: §a" + std::to_string(worker->getGeneratedCount()) +
                               "§7, loaded §a" + std::to_string(worker->getLoadedCount()) +
                               "§7, saved §a" + std::to_string(worker->getSavedCount()));
        }

        sender.sendMessage("§6Fluid queue: §a" + std::to_string(mHandler.getLevel().getScheduledFluidCount()) +
                           "§7, deferred last tick §a" +
                           std::to_string(mHandler.getLevel().getDeferredFluidCount()));
        sender.sendMessage("§6Net queues: §aout " +
                           std::to_string(mHandler.getNetworkHandler().getPendingOutboundCount()) +
                           "§7, in §a" + std::to_string(mHandler.getNetworkHandler().getPendingInboundCount()));
        sender.sendMessage("§6Tick: §a" + std::to_string(mHandler.getCurrentTick()));
    }

    return true;
}
