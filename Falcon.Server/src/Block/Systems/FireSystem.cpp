#include "Block/Systems/FireSystem.h"

#include "Actor/MobEffect.h"
#include "Actor/ServerActor.h"
#include "Actor/ServerPlayer.h"
#include "Block/BlockData.h"
#include "Block/BlockState.h"
#include "Block/Systems/RedstoneSystem.h"
#include "Level/Level.h"
#include "Level/LevelChunk.h"
#include "Network/Handler/ServerNetworkHandler.h"
#include "Protocol/Types/StartGameTypes.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
    struct ScheduledPosition {
        int32_t x;
        int32_t y;
        int32_t z;
    };

    int64_t gTick = 0;
    std::unordered_map<int64_t, int64_t> gScheduled;
    std::map<int64_t, std::vector<ScheduledPosition>> gBuckets;

    const char *UNBURNABLE_BLOCKS[] = {
            "minecraft:crimson_button",
            "minecraft:crimson_door",
            "minecraft:crimson_double_slab",
            "minecraft:crimson_fence",
            "minecraft:crimson_fence_gate",
            "minecraft:crimson_planks",
            "minecraft:crimson_pressure_plate",
            "minecraft:crimson_slab",
            "minecraft:crimson_stairs",
            "minecraft:crimson_trapdoor",
            "minecraft:crimson_wall_sign",
            "minecraft:warped_button",
            "minecraft:warped_door",
            "minecraft:warped_double_slab",
            "minecraft:warped_fence",
            "minecraft:warped_fence_gate",
            "minecraft:warped_planks",
            "minecraft:warped_pressure_plate",
            "minecraft:warped_slab",
            "minecraft:warped_stairs",
            "minecraft:warped_trapdoor",
            "minecraft:ice"
    };

    const int NEIGHBOUR_BOUND_HORIZONTAL = 300;
    const int NEIGHBOUR_BOUND_VERTICAL = 250;
    const int SPREAD_BASE_CHANCE = 100;
    const int SPREAD_HEIGHT_PENALTY = 100;
    const int SPREAD_CHANCE_BONUS = 40;
    const int SPREAD_DIFFICULTY_FACTOR = 7;
    const int SPREAD_AGE_OFFSET = 30;
    const int MAX_EXTRA_DELAY = 10;
    const int SUPPORTED_FADE_AGE = 3;

    std::mt19937 &fireRandom() {
        static std::mt19937 generator{std::random_device{}()};
        return generator;
    }

    int nextInt(int bound) {
        if (bound <= 1)
            return 0;

        std::uniform_int_distribution<int> distribution(0, bound - 1);
        return distribution(fireRandom());
    }

    bool endsWith(const std::string &value, const std::string &suffix) {
        if (value.size() < suffix.size())
            return false;

        return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    bool isChunkReady(ServerNetworkHandler &owner, const Vector3i &position) {
        if (position.y < LevelChunk::MIN_Y || position.y > LevelChunk::MAX_Y)
            return false;

        return owner.getLevel().isChunkResident(position.x >> 4, position.z >> 4);
    }

    BlockState stateAt(ServerNetworkHandler &owner, const Vector3i &position) {
        if (!isChunkReady(owner, position))
            return BlockState("minecraft:air");

        return owner.getLevel().getBlockState(position.x, position.y, position.z);
    }

    int stateInt(const BlockState &state, const std::string &key, int fallback) {
        const Tag *tag = state.mStates.get(key);
        if (tag == nullptr || tag->getType() != Tag::Type::Int)
            return fallback;

        return tag->asInt();
    }

    std::string stateString(const BlockState &state, const std::string &key) {
        const Tag *tag = state.mStates.get(key);
        if (tag == nullptr || tag->getType() != Tag::Type::String)
            return std::string();

        return tag->asString();
    }

    Vector3i relative(const Vector3i &position, int dx, int dy, int dz) {
        return Vector3i(position.x + dx, position.y + dy, position.z + dz);
    }

    bool isSolid(const std::string &identifier) {
        const BlockData *data = BlockDataTable::find(identifier.c_str());
        return data != nullptr && data->mSolid;
    }

    bool isTopFacingSurfaceSolid(const BlockState &state) {
        const std::string &identifier = state.mName;

        if (endsWith(identifier, "_stairs"))
            return false;

        if (endsWith(identifier, "_slab") || endsWith(identifier, "_double_slab"))
            return stateString(state, "minecraft:vertical_half") == "top";

        if (identifier == "minecraft:snow_layer")
            return false;

        if (endsWith(identifier, "_fence_gate") || endsWith(identifier, "trapdoor"))
            return false;

        if (identifier == "minecraft:moss_carpet" || identifier == "minecraft:azalea")
            return false;

        return isSolid(identifier);
    }

    bool burnsForever(const std::string &identifier) {
        return identifier == "minecraft:netherrack" || identifier == "minecraft:magma";
    }

    bool seesSky(Level &level, const Vector3i &position) {
        return level.getHeightAt(position.x, position.z) <= position.y;
    }

    bool canNeighbourBurn(ServerNetworkHandler &owner, const Vector3i &position) {
        static const int OFFSETS[6][3] = {
                {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}
        };

        for (const auto &offset: OFFSETS) {
            const Vector3i side = relative(position, offset[0], offset[1], offset[2]);
            if (FireSystem::getBurnChance(stateAt(owner, side).mName) > 0)
                return true;
        }

        return false;
    }

    bool isFireTickEnabled(ServerNetworkHandler &owner) {
        return owner.getLevel().getGameRules().getBool("dofiretick");
    }

    void extinguish(ServerNetworkHandler &owner, const Vector3i &position) {
        const BlockState previous = stateAt(owner, position);
        RedstoneSystem::setBlockState(owner, position, BlockState("minecraft:air"));
        RedstoneSystem::onBlockBroken(owner, position, previous);
    }

    void setFire(ServerNetworkHandler &owner, const Vector3i &position, const std::string &identifier, int age) {
        Tag states = Tag::ofCompound();
        states.putInt("age", std::clamp(age, 0, FireSystem::MAX_AGE));

        const BlockState placed(identifier, states);
        RedstoneSystem::setBlockState(owner, position, placed);
        RedstoneSystem::onBlockPlaced(owner, position, placed);
    }

    bool checkRain(ServerNetworkHandler &owner, const Vector3i &position) {
        Level &level = owner.getLevel();
        if (!level.isRaining())
            return false;

        if (burnsForever(stateAt(owner, relative(position, 0, -1, 0)).mName))
            return false;

        if (!seesSky(level, position) && !seesSky(level, relative(position, 1, 0, 0))
            && !seesSky(level, relative(position, -1, 0, 0)) && !seesSky(level, relative(position, 0, 0, 1))
            && !seesSky(level, relative(position, 0, 0, -1)))
            return false;

        extinguish(owner, position);
        return true;
    }

    int chanceOfNeighboursEncouragingFire(ServerNetworkHandler &owner, const Vector3i &position) {
        if (stateAt(owner, position).mName != "minecraft:air")
            return 0;

        int chance = 0;
        chance = std::max(chance, FireSystem::getBurnChance(stateAt(owner, relative(position, 1, 0, 0)).mName));
        chance = std::max(chance, FireSystem::getBurnChance(stateAt(owner, relative(position, -1, 0, 0)).mName));
        chance = std::max(chance, FireSystem::getBurnChance(stateAt(owner, relative(position, 0, -1, 0)).mName));
        chance = std::max(chance, FireSystem::getBurnChance(stateAt(owner, relative(position, 0, 1, 0)).mName));
        chance = std::max(chance, FireSystem::getBurnChance(stateAt(owner, relative(position, 0, 0, 1)).mName));
        chance = std::max(chance, FireSystem::getBurnChance(stateAt(owner, relative(position, 0, 0, -1)).mName));
        return chance;
    }

    void tryToCatchBlockOnFire(ServerNetworkHandler &owner, const Vector3i &position, int bound, int age) {
        const BlockState state = stateAt(owner, position);
        const int burnAbility = FireSystem::getBurnAbility(state.mName);

        if (nextInt(bound) >= burnAbility)
            return;

        if (nextInt(age + 10) < 5) {
            setFire(owner, position, "minecraft:fire", std::min(age + nextInt(5) / 4, FireSystem::MAX_AGE));
            FireSystem::scheduleUpdate(owner, position, FireSystem::TICK_RATE);
            return;
        }

        extinguish(owner, position);
    }

    void touchActor(ServerNetworkHandler &owner, ServerActor &actor) {
        if (actor.isDead() || actor.isProjectile())
            return;

        if (actor.hasEffect(MobEffectId::FireResistance))
            return;

        const Vector3f position = actor.getPosition();
        const Vector3i block((int32_t) std::floor(position.x), (int32_t) std::floor(position.y),
                             (int32_t) std::floor(position.z));

        if (!FireSystem::matches(stateAt(owner, block).mName))
            return;

        actor.hurt(owner, FireSystem::CONTACT_DAMAGE, nullptr);

        if (actor.getFireTicks() < FireSystem::COMBUST_TICKS) {
            actor.setFireTicks(FireSystem::COMBUST_TICKS);
            owner.syncActorFlags(actor);
        }
    }

    void touchPlayer(ServerNetworkHandler &owner, ServerPlayer &player) {
        if (!player.isSpawned() || player.isDead())
            return;

        const int32_t gameType = player.getGameType();
        if (gameType == (int32_t) GameType::Creative || gameType == (int32_t) GameType::Spectator)
            return;

        if (player.hasEffect(MobEffectId::FireResistance))
            return;

        const Vector3f position = player.getPosition();
        const Vector3i block((int32_t) std::floor(position.x), (int32_t) std::floor(position.y),
                             (int32_t) std::floor(position.z));

        if (!FireSystem::matches(stateAt(owner, block).mName))
            return;

        owner.applyDamage(player, FireSystem::CONTACT_DAMAGE, "death.attack.inFire", {player.getName()});

        if (player.getFireTicks() < FireSystem::COMBUST_TICKS)
            player.setFireTicks(FireSystem::COMBUST_TICKS);
    }
}

bool FireSystem::matches(const std::string &identifier) {
    return identifier == "minecraft:fire" || identifier == "minecraft:soul_fire";
}

int FireSystem::getBurnChance(const std::string &identifier) {
    if (!canBeIgnitedAgainst(identifier))
        return UNBURNABLE;

    const BlockData *data = BlockDataTable::find(identifier.c_str());
    return data == nullptr ? 0 : (int) data->mBurnChance;
}

int FireSystem::getBurnAbility(const std::string &identifier) {
    const BlockData *data = BlockDataTable::find(identifier.c_str());
    return data == nullptr ? 0 : (int) data->mBurnAbility;
}

bool FireSystem::canBeIgnitedAgainst(const std::string &identifier) {
    for (const char *unburnable: UNBURNABLE_BLOCKS) {
        if (identifier == unburnable)
            return false;
    }

    return true;
}

bool FireSystem::canSurviveAt(ServerNetworkHandler &owner, const Vector3i &position) {
    return isTopFacingSurfaceSolid(stateAt(owner, relative(position, 0, -1, 0)))
           || canNeighbourBurn(owner, position);
}

bool FireSystem::ignite(ServerNetworkHandler &owner, const Vector3i &position) {
    if (!isChunkReady(owner, position))
        return false;

    if (stateAt(owner, position).mName != "minecraft:air")
        return false;

    if (!canSurviveAt(owner, position))
        return false;

    const std::string below = stateAt(owner, relative(position, 0, -1, 0)).mName;
    const bool soul = below == "minecraft:soul_sand" || below == "minecraft:soul_soil";

    setFire(owner, position, soul ? "minecraft:soul_fire" : "minecraft:fire", 0);
    scheduleUpdate(owner, position, TICK_RATE + nextInt(MAX_EXTRA_DELAY));
    return true;
}

void FireSystem::onNormalUpdate(ServerNetworkHandler &owner, const Vector3i &position, const BlockState &state) {
    if (!matches(state.mName))
        return;

    const std::string below = stateAt(owner, relative(position, 0, -1, 0)).mName;

    if (state.mName == "minecraft:fire" && (below == "minecraft:soul_sand" || below == "minecraft:soul_soil")) {
        setFire(owner, position, "minecraft:soul_fire", stateInt(state, "age", 0));
        return;
    }

    if (!canSurviveAt(owner, position)) {
        extinguish(owner, position);
        return;
    }

    if (isFireTickEnabled(owner) && gScheduled.count(RedstoneSystem::packPosition(position)) == 0)
        scheduleUpdate(owner, position, TICK_RATE);

    checkRain(owner, position);
}

void FireSystem::onScheduledUpdate(ServerNetworkHandler &owner, const Vector3i &position, const BlockState &state) {
    if (!matches(state.mName) || !isFireTickEnabled(owner))
        return;

    if (!canSurviveAt(owner, position)) {
        extinguish(owner, position);
        return;
    }

    if (checkRain(owner, position))
        return;

    const Vector3i below = relative(position, 0, -1, 0);
    const BlockState belowState = stateAt(owner, below);
    const bool forever = state.mName == "minecraft:soul_fire" || burnsForever(belowState.mName);
    const int age = stateInt(state, "age", 0);

    if (age < MAX_AGE)
        setFire(owner, position, state.mName, std::min(age + nextInt(3), MAX_AGE));

    scheduleUpdate(owner, position, TICK_RATE + nextInt(MAX_EXTRA_DELAY));

    if (!forever && !canNeighbourBurn(owner, position)) {
        if (!isTopFacingSurfaceSolid(belowState) || age > SUPPORTED_FADE_AGE)
            extinguish(owner, position);
        return;
    }

    if (!forever && getBurnAbility(belowState.mName) == 0 && age == MAX_AGE && nextInt(4) == 0) {
        extinguish(owner, position);
        return;
    }

    tryToCatchBlockOnFire(owner, relative(position, 1, 0, 0), NEIGHBOUR_BOUND_HORIZONTAL, age);
    tryToCatchBlockOnFire(owner, relative(position, -1, 0, 0), NEIGHBOUR_BOUND_HORIZONTAL, age);
    tryToCatchBlockOnFire(owner, below, NEIGHBOUR_BOUND_VERTICAL, age);
    tryToCatchBlockOnFire(owner, relative(position, 0, 1, 0), NEIGHBOUR_BOUND_VERTICAL, age);
    tryToCatchBlockOnFire(owner, relative(position, 0, 0, 1), NEIGHBOUR_BOUND_HORIZONTAL, age);
    tryToCatchBlockOnFire(owner, relative(position, 0, 0, -1), NEIGHBOUR_BOUND_HORIZONTAL, age);

    const int difficulty = (int) owner.getProperties().getDifficulty();

    for (int32_t x = position.x - 1; x <= position.x + 1; ++x) {
        for (int32_t z = position.z - 1; z <= position.z + 1; ++z) {
            for (int32_t y = position.y - 1; y <= position.y + 4; ++y) {
                if (x == position.x && y == position.y && z == position.z)
                    continue;

                int bound = SPREAD_BASE_CHANCE;
                if (y > position.y + 1)
                    bound += (y - (position.y + 1)) * SPREAD_HEIGHT_PENALTY;

                const Vector3i target(x, y, z);
                const int chance = chanceOfNeighboursEncouragingFire(owner, target);
                if (chance <= 0)
                    continue;

                const int threshold = (chance + SPREAD_CHANCE_BONUS + difficulty * SPREAD_DIFFICULTY_FACTOR)
                                      / (age + SPREAD_AGE_OFFSET);
                if (threshold <= 0 || nextInt(bound) > threshold)
                    continue;

                setFire(owner, target, "minecraft:fire", std::min(age + nextInt(5) / 4, MAX_AGE));
                scheduleUpdate(owner, target, TICK_RATE);
            }
        }
    }
}

void FireSystem::scheduleUpdate(ServerNetworkHandler &owner, const Vector3i &position, int64_t delay) {
    (void) owner;

    if (delay < 1)
        delay = 1;

    const int64_t key = RedstoneSystem::packPosition(position);
    const int64_t target = gTick + delay;

    const auto it = gScheduled.find(key);
    if (it != gScheduled.end() && it->second <= target)
        return;

    gScheduled[key] = target;

    ScheduledPosition entry;
    entry.x = position.x;
    entry.y = position.y;
    entry.z = position.z;
    gBuckets[target].push_back(entry);
}

void FireSystem::tick(ServerNetworkHandler &owner) {
    ++gTick;

    std::vector<ScheduledPosition> due;

    while (!gBuckets.empty()) {
        const auto it = gBuckets.begin();
        if (it->first > gTick)
            break;

        for (const ScheduledPosition &entry: it->second)
            due.push_back(entry);

        gBuckets.erase(it);
    }

    for (const ScheduledPosition &entry: due) {
        const Vector3i position(entry.x, entry.y, entry.z);
        const int64_t key = RedstoneSystem::packPosition(position);

        const auto scheduledIt = gScheduled.find(key);
        if (scheduledIt == gScheduled.end() || scheduledIt->second > gTick)
            continue;

        gScheduled.erase(scheduledIt);
        onScheduledUpdate(owner, position, stateAt(owner, position));
    }

    for (auto &entry: owner.getPlayers())
        touchPlayer(owner, entry.second);

    for (auto &entry: owner.getActors()) {
        ServerActor *actor = entry.second.get();
        if (actor != nullptr)
            touchActor(owner, *actor);
    }
}
