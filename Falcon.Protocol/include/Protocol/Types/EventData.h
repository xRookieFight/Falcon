#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class EventDataType : int32_t {
    AchievementAwarded,
    EntityInteract,
    PortalBuilt,
    PortalUsed,
    MobKilled,
    CauldronUsed,
    PlayerDied,
    BossKilled,
    AgentCommand,
    AgentCreated,
    PatternRemoved,
    SlashCommandExecuted,
    FishBucketed,
    MobBorn,
    PetDied,
    CauldronInteract,
    ComposterInteract,
    BellUsed,
    EntityDefinitionTrigger,
    RaidUpdate,
    MovementAnomaly,
    MovementCorrected,
    ExtractHoney,
    TargetBlockHit,
    PiglinBarter,
    CopperWaxedOrUnwaxed,
    CodeBuilderAction,
    CodeBuilderScoreboard,
    StriderRiddenInLavaInOverworld,
    SneakCloseToSculkSensor,
    CarefulRestoration,
    ItemUsedEvent,
};

enum class AgentResult : int32_t {
    ActionFail,
    ActionSuccess,
    QueryResultFalse,
    QueryResultTrue,
};

class EventData {
public:
    EventDataType mType = EventDataType::AchievementAwarded;

    int32_t mAchievementId = 0;

    int32_t mInteractionType = 0;
    int32_t mLegacyEntityTypeId = 0;
    int32_t mEntityVariant = 0;
    int32_t mPaletteColor = 0;

    int32_t mDimensionId = 0;
    int32_t mFromDimensionId = 0;
    int32_t mToDimensionId = 0;

    int64_t mKillerUniqueEntityId = 0;
    int64_t mVictimUniqueEntityId = 0;
    int32_t mEntityDamageCause = 0;
    int32_t mVillagerTradeTier = 0;
    std::string mVillagerDisplayName;

    int32_t mPotionId = 0;
    int32_t mColor = 0;
    int32_t mFillLevel = 0;

    int32_t mAttackerEntityId = 0;

    int64_t mBossUniqueEntityId = 0;
    int32_t mPlayerPartySize = 0;
    int32_t mBossEntityType = 0;

    AgentResult mAgentResult = AgentResult::ActionFail;
    int32_t mAgentDataValue = 0;
    std::string mAgentCommand;
    std::string mAgentDataKey;
    std::string mAgentOutput;

    int32_t mPatternItemId = 0;
    int32_t mPatternAuxValue = 0;
    int32_t mPatternsSize = 0;
    int32_t mPatternIndex = 0;
    int32_t mPatternColor = 0;

    std::string mSlashCommandName;
    int32_t mSlashSuccessCount = 0;
    int32_t mSlashErrorCount = 0;
    std::vector<std::string> mSlashOutputMessages;

    int32_t mFishPattern = 0;
    int32_t mFishPreset = 0;
    int32_t mFishBucketedEntityType = 0;
    bool mFishReleaseEvent = false;
};
