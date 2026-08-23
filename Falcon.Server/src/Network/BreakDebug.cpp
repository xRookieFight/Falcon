#include "Network/BreakDebug.h"

#include "Actor/ServerPlayer.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>

namespace {
    std::mutex gBreakDebugMutex;

    std::string formatTime() {
        const auto now = std::chrono::system_clock::now();
        const std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &time);
#else
        localtime_r(&time, &localTime);
#endif

        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) %
                                  1000;
        std::ostringstream output;
        output << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3)
               << milliseconds.count();
        return output.str();
    }
}

void BreakDebug::log(const std::string &event, const ServerPlayer *player, const std::string &details) {
    std::lock_guard<std::mutex> lock(gBreakDebugMutex);
    std::ofstream file("break.txt", std::ios::out | std::ios::app);
    if (!file)
        return;

    file << '[' << formatTime() << "][thread=" << std::this_thread::get_id() << "] " << event;
    if (player != nullptr) {
        const Vector3f &position = player->getPosition();
        const Vector3f &rotation = player->getRotation();
        const Vector3f &motion = player->getMotion();
        const ItemStack &held = player->getInventory().getItemInHand();
        file << " player=" << player->getName() << " runtime=" << player->getRuntimeId()
             << " connection=" << player->getNetworkIdentifier().toString() << " state="
             << (int) player->getLoginState() << " gamemode=" << player->getGameType() << " dead="
             << (player->isDead() ? 1 : 0) << " pos=(" << std::fixed << std::setprecision(4) << position.x << ','
             << position.y << ',' << position.z << ") rot=(" << rotation.x << ',' << rotation.y << ',' << rotation.z
             << ") motion=(" << motion.x << ',' << motion.y << ',' << motion.z << ") onGround="
             << (player->isOnGround() ? 1 : 0) << " breaking=" << (player->isBreakingBlock() ? 1 : 0)
             << " breakPos=(" << player->getBreakingBlockPosition().x << ',' << player->getBreakingBlockPosition().y
             << ',' << player->getBreakingBlockPosition().z << ") face=" << player->getBreakingFace()
             << " progress=" << player->getBreakProgress() << " held="
             << (held.isAir() || held.mDefinition == nullptr ? "minecraft:air" : held.mDefinition->getIdentifier())
             << " heldCount=" << held.mCount << " heldDamage=" << held.mDamage;
    }
    if (!details.empty())
        file << " " << details;
    file << '\n';
    file.flush();
}
