#include "Block/Systems/CommandBlockSystem.h"

#include "Actor/ServerPlayer.h"
#include "Block/Actor/CommandBlockActor.h"
#include "Block/Blocks/CommandBlock.h"
#include "Block/BlockState.h"
#include "Command/CommandMap.h"
#include "Command/ServerCommandOrigin.h"
#include "Core/Debug/BedrockLog.h"
#include "Level/Level.h"
#include "Network/Handler/BlockActionHandler.h"
#include "Network/Handler/ServerNetworkHandler.h"
#include "Protocol/Packets/BlockActorDataPacket.h"
#include "Protocol/Packets/CommandBlockUpdatePacket.h"
#include "Protocol/Types/StartGameTypes.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace {
    const int MAX_CHAIN_LENGTH = 1024;

    std::unordered_map<int64_t, CommandBlockActor> gCommandBlocks;

    std::unordered_map<NetworkIdentifier, std::unordered_map<int64_t, uint64_t>,
                       NetworkIdentifier::Hasher> gSyncedRevisions;

    class CommandBlockOrigin final : public CommandOrigin {
    public:
        explicit CommandBlockOrigin(const std::string &name) : mName(name)
        {
        }

        const std::string &getSenderName() const override
        {
            return mName;
        }

        bool isPlayer() const override
        {
            return false;
        }

        ServerPlayer *asPlayer() override
        {
            return nullptr;
        }

        void sendMessage(const std::string &message) override
        {
            if (!mOutput.empty())
                mOutput += "\n";

            mOutput += message;
        }

        void sendTranslation(const std::string &key, const std::vector<std::string> &parameters) override
        {
            std::string message = key;
            for (const std::string &parameter: parameters)
                message += " " + parameter;

            sendMessage(message);
        }

        CommandPermission getCommandPermission() const override
        {
            return CommandPermission::Internal;
        }

        const std::string &getOutput() const
        {
            return mOutput;
        }

    private:
        std::string mName;
        std::string mOutput;
    };

    Vector3i offsetForFacing(int32_t facing)
    {
        switch (facing) {
            case 0:
                return Vector3i(0, -1, 0);
            case 1:
                return Vector3i(0, 1, 0);
            case 2:
                return Vector3i(0, 0, -1);
            case 3:
                return Vector3i(0, 0, 1);
            case 4:
                return Vector3i(-1, 0, 0);
            case 5:
                return Vector3i(1, 0, 0);
            default:
                return Vector3i(0, 0, 0);
        }
    }

    int32_t facingOf(const BlockState &state)
    {
        const Tag *facing = state.mStates.get("facing_direction");
        if (facing == nullptr)
            return 0;

        if (facing->getType() == Tag::Type::Int)
            return facing->asInt();

        if (facing->getType() == Tag::Type::Byte)
            return (int32_t) facing->asByte();

        return 0;
    }

    Vector3i addOffset(const Vector3i &position, const Vector3i &offset)
    {
        return Vector3i(position.x + offset.x, position.y + offset.y, position.z + offset.z);
    }

    Vector3i frontOf(const BlockState &state, const Vector3i &position)
    {
        return addOffset(position, offsetForFacing(facingOf(state)));
    }

    Vector3i behindOf(const BlockState &state, const Vector3i &position)
    {
        const Vector3i offset = offsetForFacing(facingOf(state));
        return Vector3i(position.x - offset.x, position.y - offset.y, position.z - offset.z);
    }

    void setConditionMet(ServerNetworkHandler &owner, CommandBlockActor &actor, const BlockState &state)
    {
        if (!actor.isConditional()) {
            actor.mConditionMet = true;
            return;
        }

        const Vector3i behind = behindOf(state, actor.mPosition);
        const BlockState behindState = owner.getLevel().getBlockState(behind.x, behind.y, behind.z);
        if (!CommandBlock::matches(behindState.mName)) {
            actor.mConditionMet = false;
            return;
        }

        const CommandBlockActor *behindActor = CommandBlockSystem::find(behind);
        actor.mConditionMet = behindActor != nullptr && behindActor->mSuccessCount > 0;
    }

    bool execute(ServerNetworkHandler &owner, CommandBlockActor &actor, int chain)
    {
        if (chain > MAX_CHAIN_LENGTH)
            return false;

        const Vector3i position = actor.mPosition;
        const BlockState state = owner.getLevel().getBlockState(position.x, position.y, position.z);
        if (!CommandBlock::matches(state.mName)) {
            CommandBlockSystem::remove(position);
            return false;
        }

        actor.setMode(CommandBlockActor::modeFromBlockName(state.mName));

        const Vector3i behind = behindOf(state, position);
        const BlockState behindState = owner.getLevel().getBlockState(behind.x, behind.y, behind.z);
        if (CommandBlock::matches(behindState.mName)) {
            const CommandBlockActor *behindActor = CommandBlockSystem::find(behind);
            if (actor.isConditional() && (behindActor == nullptr || behindActor->mSuccessCount == 0)) {
                const Vector3i next = frontOf(state, position);
                const BlockState nextState = owner.getLevel().getBlockState(next.x, next.y, next.z);
                if (nextState.mName == "minecraft:chain_command_block")
                    CommandBlockSystem::trigger(owner, next, chain + 1);

                return true;
            }
        }

        const std::string previousOutput = actor.mLastOutput;
        const int32_t previousSuccessCount = actor.mSuccessCount;

        const int64_t currentTick = owner.getCurrentTick();
        if (actor.mLastExecution != currentTick) {
            setConditionMet(owner, actor, state);

            if (actor.mConditionMet && (actor.isAuto() || actor.isPowered())) {
                if (!actor.mCommand.empty()) {
                    CommandBlockOrigin origin(actor.mCustomName.empty() ? std::string("@") : actor.mCustomName);
                    const bool success = owner.getCommands().dispatch(origin, actor.mCommand);
                    actor.mSuccessCount = success ? 1 : 0;

                    if (actor.mTrackOutput)
                        actor.mLastOutput = origin.getOutput();
                    else
                        actor.mLastOutput.clear();
                }

                const Vector3i next = frontOf(state, position);
                const BlockState nextState = owner.getLevel().getBlockState(next.x, next.y, next.z);
                if (nextState.mName == "minecraft:chain_command_block")
                    CommandBlockSystem::trigger(owner, next, chain + 1);
            }

            actor.mLastExecution = currentTick;
            actor.mLastOutputCommandMode = (int32_t) actor.getMode();
            actor.mLastOutputCondionalMode = actor.isConditional();
            actor.mLastOutputRedstoneMode = !actor.isAuto();
        } else {
            actor.mSuccessCount = 0;
        }

        if (actor.mLastOutput != previousOutput || actor.mSuccessCount != previousSuccessCount)
            actor.mRevision++;

        return true;
    }
}

int64_t CommandBlockSystem::packPosition(const Vector3i &position)
{
    const int64_t x = (int64_t) (position.x & 0x3ffffff);
    const int64_t y = (int64_t) (position.y & 0xfff);
    const int64_t z = (int64_t) (position.z & 0x3ffffff);
    return (x << 38) | (y << 26) | z;
}

CommandBlockActor *CommandBlockSystem::find(const Vector3i &position)
{
    const auto it = gCommandBlocks.find(packPosition(position));
    if (it == gCommandBlocks.end())
        return nullptr;

    return &it->second;
}

CommandBlockActor &CommandBlockSystem::getOrCreate(ServerNetworkHandler &owner, const Vector3i &position)
{
    const int64_t key = packPosition(position);
    const auto it = gCommandBlocks.find(key);
    if (it != gCommandBlocks.end())
        return it->second;

    const BlockState state = owner.getLevel().getBlockState(position.x, position.y, position.z);

    CommandBlockActor actor;
    actor.setState(state);
    actor.mPosition = position;
    actor.setMode(CommandBlockActor::modeFromBlockName(state.mName));

    return gCommandBlocks.emplace(key, actor).first->second;
}

void CommandBlockSystem::remove(const Vector3i &position)
{
    gCommandBlocks.erase(packPosition(position));
}

void CommandBlockSystem::onCommandBlockUpdate(ServerNetworkHandler &owner, ServerPlayer &player,
                                              const CommandBlockUpdatePacket &packet)
{
    if (!packet.mBlock)
        return;

    if (!player.isOp() && player.getGameType() != (int32_t) GameType::Creative) {
        LOG_INFO(LogAreaID::Server, "Rejected command block update from %s: missing permission",
                 player.getName().c_str());
        return;
    }

    const Vector3i position = packet.mBlockPosition;
    const BlockState state = owner.getLevel().getBlockState(position.x, position.y, position.z);
    if (!CommandBlock::matches(state.mName))
        return;

    CommandBlockActor &actor = getOrCreate(owner, position);
    actor.setState(state);
    actor.mPosition = position;
    actor.setMode(CommandBlockActor::modeFromBlockName(state.mName));
    actor.setCommand(packet.mCommand);
    actor.mCustomName = packet.mName;
    actor.mTrackOutput = packet.mOutputTracked;
    actor.mLastOutput = packet.mOutputTracked ? packet.mLastOutput : std::string();
    actor.mTickDelay = (int32_t) packet.mTickDelay;
    actor.mExecutingOnFirstTick = packet.mExecutingOnFirstTick;
    actor.setConditional(packet.mConditional);
    actor.setAuto(!packet.mRedstoneMode);
    actor.mCurrentTick = 0;
    actor.mRevision++;

    broadcastData(owner, actor);

    if (actor.getMode() == CommandBlockActorMode::Normal && actor.isAuto() && actor.mExecutingOnFirstTick)
        execute(owner, actor, 0);
}

void CommandBlockSystem::setPowered(ServerNetworkHandler &owner, const Vector3i &position, bool powered)
{
    CommandBlockActor *actor = find(position);
    if (actor == nullptr)
        return;

    const bool wasPowered = actor->isPowered();
    actor->setPowered(powered);
    if (wasPowered == powered)
        return;

    actor->mRevision++;

    if (!powered)
        return;

    if (actor->getMode() == CommandBlockActorMode::Normal)
        execute(owner, *actor, 0);
}

void CommandBlockSystem::trigger(ServerNetworkHandler &owner, const Vector3i &position, int chain)
{
    CommandBlockActor *actor = find(position);
    if (actor == nullptr)
        return;

    execute(owner, *actor, chain);
}

void CommandBlockSystem::broadcastData(ServerNetworkHandler &owner, const CommandBlockActor &actor)
{
    BlockActorDataPacket data;
    data.mBlockPosition = actor.mPosition;
    data.mData = actor.getSpawnCompound();

    const Vector3f center((float) actor.mPosition.x + 0.5f, (float) actor.mPosition.y + 0.5f,
                          (float) actor.mPosition.z + 0.5f);
    BlockActionHandler::broadcastToViewers(owner, center, data);

    const int64_t key = packPosition(actor.mPosition);
    const int32_t chunkX = actor.mPosition.x >> 4;
    const int32_t chunkZ = actor.mPosition.z >> 4;
    const int64_t chunkKey = ((int64_t) chunkX << 32) | (uint32_t) chunkZ;

    for (auto &entry: owner.getPlayers()) {
        if (entry.second.isSpawned() && entry.second.getSentChunks().count(chunkKey) != 0)
            gSyncedRevisions[entry.first][key] = actor.mRevision;
    }
}

void CommandBlockSystem::tickCommandBlocks(ServerNetworkHandler &owner)
{
    std::vector<Vector3i> stale;

    for (auto &entry: gCommandBlocks) {
        CommandBlockActor &actor = entry.second;
        const Vector3i position = actor.mPosition;
        const BlockState state = owner.getLevel().getBlockState(position.x, position.y, position.z);

        if (!CommandBlock::matches(state.mName)) {
            stale.push_back(position);
            continue;
        }

        actor.setState(state);
        actor.setMode(CommandBlockActor::modeFromBlockName(state.mName));
        actor.setConditional(state.mStates.getBool("conditional_bit", actor.isConditional()));

        if (actor.getMode() != CommandBlockActorMode::Repeating)
            continue;

        if (actor.mCurrentTick++ < actor.mTickDelay)
            continue;

        actor.mCurrentTick = 0;
        execute(owner, actor, 0);
    }

    for (const Vector3i &position: stale) {
        const int64_t key = packPosition(position);
        gCommandBlocks.erase(key);
        for (auto &entry: gSyncedRevisions)
            entry.second.erase(key);
    }

    for (auto it = gSyncedRevisions.begin(); it != gSyncedRevisions.end();) {
        if (owner.getPlayers().count(it->first) == 0)
            it = gSyncedRevisions.erase(it);
        else
            ++it;
    }

    for (auto &playerEntry: owner.getPlayers()) {
        ServerPlayer &player = playerEntry.second;
        if (!player.isSpawned())
            continue;

        std::unordered_map<int64_t, uint64_t> &synced = gSyncedRevisions[playerEntry.first];

        for (auto &blockEntry: gCommandBlocks) {
            const CommandBlockActor &actor = blockEntry.second;
            const int32_t chunkX = actor.mPosition.x >> 4;
            const int32_t chunkZ = actor.mPosition.z >> 4;
            const int64_t chunkKey = ((int64_t) chunkX << 32) | (uint32_t) chunkZ;

            if (player.getSentChunks().count(chunkKey) == 0)
                continue;

            const auto syncedIt = synced.find(blockEntry.first);
            if (syncedIt != synced.end() && syncedIt->second == actor.mRevision)
                continue;

            BlockActorDataPacket data;
            data.mBlockPosition = actor.mPosition;
            data.mData = actor.getSpawnCompound();
            owner.getNetworkHandler().send(playerEntry.first, data, owner.getCodecContext());

            synced[blockEntry.first] = actor.mRevision;
        }
    }
}
