#include "Server.h"

#include "BuildInfo.h"
#include "Core/Utility/UUID.h"
#include "Core/Debug/BedrockLog.h"
#include "Core/Debug/ContentLogEndPoint.h"
#include "Core/Debug/FileLogEndPoint.h"
#include "Level/Generator/Biome/BiomeChunkGenDataRegistry.h"
#include "Network/Handler/ServerNetworkHandler.h"
#include "Network/TransportFactory.h"
#include "Server/PropertiesSettings.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>

static const char *PROPERTIES_FILE = "server.properties";
static const char *PROFILER_CONFIG_FILE = "bootstrap.json";
static const char *CDN_CONFIG_FILE = "cdn_config.json";

static const char *LOADING_WORLD_BANNER =
        "\n\n"
        "#####################################################\n"
        "#                                                   #\n"
        "#               LOADING VANILLA WORLD               #\n"
        "#                                                   #\n"
        "#####################################################";

static std::atomic<bool> gRunning(true);

static void requestShutdown(int signalNumber) {
    (void) signalNumber;
    gRunning.store(false);
}

static bool fileExists(const char *path);

static void setupServerLogging() {
    static const char *LOG_FILE = "server.log";

    if (!fileExists(LOG_FILE)) {
        printf("NO LOG FILE! - setting up server logging...\n");
        fflush(stdout);
    }

    BedrockLog::setLogLevel(LogLevel::Info);
    BedrockLog::addEndPoint(std::make_shared<ContentLogEndPoint>());

    std::shared_ptr<FileLogEndPoint> fileEndPoint = std::make_shared<FileLogEndPoint>(LOG_FILE);
    if (fileEndPoint->isOpen())
        BedrockLog::addEndPoint(fileEndPoint);
}

static std::string generateSessionId() {
    std::random_device device;
    std::mt19937_64 generator(((uint64_t) device() << 32) ^ device());
    std::uniform_int_distribution<uint64_t> distribution;

    uint64_t most = distribution(generator);
    uint64_t least = distribution(generator);

    most = (most & 0xffffffffffff0fffULL) | 0x0000000000004000ULL;
    least = (least & 0x3fffffffffffffffULL) | 0x8000000000000000ULL;

    return Uuid(most, least).toString();
}

static bool fileExists(const char *path) {
    std::ifstream file(path);
    return file.is_open();
}

static const char *toString(GameType gameType) {
    switch (gameType) {
        case GameType::Creative:
            return "Creative";
        case GameType::Adventure:
            return "Adventure";
        case GameType::Spectator:
            return "Spectator";
        default:
            return "Survival";
    }
}

static const char *toString(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::Peaceful:
            return "PEACEFUL";
        case Difficulty::Normal:
            return "NORMAL";
        case Difficulty::Hard:
            return "HARD";
        default:
            return "EASY";
    }
}

static void logStartupBanner(const PropertiesSettings &properties) {
    LOG_INFO(LogAreaID::Server, "Starting Server");
    LOG_INFO(LogAreaID::Server, "Version: %s", FalconBuildInfo::kVersion);
    LOG_INFO(LogAreaID::Server, "Session ID: %s", generateSessionId().c_str());
    LOG_INFO(LogAreaID::Server, "Build ID: %s", FalconBuildInfo::kBuildId);
    LOG_INFO(LogAreaID::Server, "Branch: %s", FalconBuildInfo::kBranch);
    LOG_INFO(LogAreaID::Server, "Commit ID: %s", FalconBuildInfo::kCommitId);
    LOG_INFO(LogAreaID::Server, "Configuration: %s", FalconBuildInfo::kConfiguration);
    LOG_INFO(LogAreaID::Server, "Contents of %s: %s", PROPERTIES_FILE, properties.getUnknownContents().c_str());
    LOG_INFO(LogAreaID::Server, "Level Name: %s", properties.getLevelName().c_str());
    LOG_INFO(LogAreaID::Server, "Profiler config ('%s') load result: success=%d, errorMessage=(null)",
             PROFILER_CONFIG_FILE, fileExists(PROFILER_CONFIG_FILE) ? 1 : 0);

    if (!fileExists(CDN_CONFIG_FILE))
        LOG_INFO(LogAreaID::Server, "No CDN config file found at: %s for dedicated server", CDN_CONFIG_FILE);

    LOG_INFO(LogAreaID::Server, "Game mode: %d %s", (int) properties.getGameType(),
             toString(properties.getGameType()));
    LOG_INFO(LogAreaID::Server, "Difficulty: %d %s", (int) properties.getDifficulty(),
             toString(properties.getDifficulty()));

    if (!properties.getContentLogConsoleOutputEnabled())
        LOG_WARN(LogAreaID::Server, "Content logging to console is disabled.  Enable it with "
                                    "content-log-console-output-enabled=true in server.properties");

    LOG_INFO(LogAreaID::Server, "%s", LOADING_WORLD_BANNER);
}

static void logTransportNotice(TransportLayer transport) {
    LOG_INFO(LogAreaID::Server, "==================== TRANSPORT =======================");
    LOG_INFO(LogAreaID::Server, "Connection type: %s", toString(transport));
    if (transport == TransportLayer::NetherNet) {
        LOG_INFO(LogAreaID::Server, "NetherNet serves its signaling endpoint over plain HTTP.");
    } else {
        LOG_INFO(LogAreaID::Server, "RakNet transport is active.");
    }
    LOG_INFO(LogAreaID::Server, "======================================================");
}

static void logTelemetryNotice() {
    LOG_INFO(LogAreaID::Server, "================ TELEMETRY MESSAGE ===================");
    LOG_INFO(LogAreaID::Server, "Server Telemetry is currently not enabled. ");
    LOG_INFO(LogAreaID::Server, "Enabling this telemetry helps us improve the game.");
    LOG_INFO(LogAreaID::Server, "%s", "");
    LOG_INFO(LogAreaID::Server, "To enable this feature, add the line 'emit-server-telemetry=true'");
    LOG_INFO(LogAreaID::Server, "to the server.properties file in the handheld/src-server directory");
    LOG_INFO(LogAreaID::Server, "======================================================");
}

void startServer(const ServerSettings &settings) {
    setupServerLogging();

    BiomeChunkGenDataRegistry::initialize();

    PropertiesSettings properties(PROPERTIES_FILE);
    logStartupBanner(properties);

    const TransportLayer transport = properties.getTransportLayer();

    if (transport == TransportLayer::Unknown) {
        LOG_FATAL(LogAreaID::Server, "Unknown 'transport' value in %s, expected 'raknet' or 'nethernet'",
                  PROPERTIES_FILE);
        BedrockLog::flush();
        return;
    }

    if (!TransportFactory::isSupported(transport)) {
        LOG_FATAL(LogAreaID::Server, "Transport %s is not supported by this build", toString(transport));
        BedrockLog::flush();
        return;
    }

    const int maxPlayers = properties.isLoaded() ? properties.getMaxPlayers() : settings.maxPlayers;
    const std::string serverName = properties.isLoaded() ? properties.getServerName() : settings.motd;

    ServerNetworkHandler networkHandler(serverName, settings.subMotd, maxPlayers, transport);
    networkHandler.setProtocolVersion(settings.protocolVersion, settings.gameVersion);
    networkHandler.setProperties(properties);

    unsigned short port = properties.isLoaded() ? properties.getServerPort() : settings.port;
    unsigned short portV6 = properties.isLoaded() ? properties.getServerPortV6() : settings.portV6;

    // the environment variable stays available so a second instance can be started without editing the file
    if (const char *portOverride = getenv("FALCON_PORT"))
        port = (unsigned short) atoi(portOverride);

    const ConnectionDefinition definition = ConnectionDefinition::createFromPorts(port, portV6, maxPlayers);

    if (!networkHandler.startServerListening(definition)) {
        LOG_FATAL(LogAreaID::Server, "Failed to start server");
        BedrockLog::flush();
        return;
    }

    LOG_INFO(LogAreaID::Server, "Server started.");
    logTransportNotice(transport);
    logTelemetryNotice();
    BedrockLog::flush();

    std::thread consoleThread([&networkHandler]() {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (!line.empty())
                networkHandler.queueConsoleCommand(line);
        }
    });
    consoleThread.detach();

    std::signal(SIGINT, requestShutdown);
    std::signal(SIGTERM, requestShutdown);

    const std::chrono::nanoseconds tickInterval(50000000);
    const std::chrono::nanoseconds catchupResetInterval(1000000000);
    std::chrono::steady_clock::time_point nextTick = std::chrono::steady_clock::now();

    while (gRunning.load()) {
        networkHandler.tick();

        nextTick += tickInterval;

        const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if (now - nextTick > catchupResetInterval)
            nextTick = now;
        else
            std::this_thread::sleep_until(nextTick);
    }

    LOG_INFO(LogAreaID::Server, "Shutting down...");
    networkHandler.stopServerListening();
    BedrockLog::shutdown();
}

int main() {
    startServer();
    return 0;
}
