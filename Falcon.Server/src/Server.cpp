#include "Server.h"

#include "Core/Debug/BedrockLog.h"
#include "Core/Debug/ContentLogEndPoint.h"
#include "Core/Debug/FileLogEndPoint.h"
#include "Network/ServerNetworkHandler.h"
#include "Server/PropertiesSettings.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

static const char *PROPERTIES_FILE = "server.properties";

static std::atomic<bool> gRunning(true);

static void requestShutdown(int signalNumber) {
    (void) signalNumber;
    gRunning.store(false);
}

static void setupServerLogging() {
    BedrockLog::setLogLevel(LogLevel::Info);
    BedrockLog::addEndPoint(std::make_shared<ContentLogEndPoint>());

    std::shared_ptr<FileLogEndPoint> fileEndPoint = std::make_shared<FileLogEndPoint>("FalconServer.log");
    if (fileEndPoint->isOpen()) {
        BedrockLog::addEndPoint(fileEndPoint);
        return;
    }

    LOG_WARN(LogAreaID::System, "NO LOG FILE! - setting up server logging...");
}

void startServer(const ServerSettings &settings) {
    setupServerLogging();

    PropertiesSettings properties(PROPERTIES_FILE);
    if (!properties.isLoaded())
        LOG_WARN(LogAreaID::Server, "No %s found, falling back to built in defaults", PROPERTIES_FILE);

    LOG_INFO(LogAreaID::Server, "Starting Server");
    LOG_INFO(LogAreaID::Server, "Version %s", settings.gameVersion.c_str());
    LOG_INFO(LogAreaID::Server, "Level Name: %s", properties.getLevelName().c_str());
    LOG_INFO(LogAreaID::Server, "Protocol Version %d", settings.protocolVersion);

    const int maxPlayers = properties.isLoaded() ? properties.getMaxPlayers() : settings.maxPlayers;
    const std::string serverName = properties.isLoaded() ? properties.getServerName() : settings.motd;

    ServerNetworkHandler networkHandler(serverName, settings.subMotd, maxPlayers);
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

    while (gRunning.load()) {
        networkHandler.tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    LOG_INFO(LogAreaID::Server, "Shutting down...");
    networkHandler.stopServerListening();
    BedrockLog::flush();
}

int main() {
    startServer();
    return 0;
}
