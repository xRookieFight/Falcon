#pragma once

#include "Core/Utility/UUID.h"

#include <cstdint>
#include <string>
#include <vector>

class SkinImageData {
public:
    int32_t mWidth = 0;
    int32_t mHeight = 0;
    std::string mData;
};

enum class AnimatedTextureType : int32_t {
    None = 0,
    Face = 1,
    Body32x32 = 2,
    Body128x128 = 3
};

enum class AnimationExpressionType : int32_t {
    Linear = 0,
    Blinking = 1
};

class SkinAnimationData {
public:
    SkinImageData mImage;
    AnimatedTextureType mTextureType = AnimatedTextureType::None;
    float mFrames = 0.0f;
    AnimationExpressionType mExpressionType = AnimationExpressionType::Linear;
};

enum class PersonaPieceType : int32_t {
    Unknown = 0,
    Skeleton = 1,
    Body = 2,
    Skin = 3,
    Bottom = 4,
    Feet = 5,
    Dress = 6,
    Top = 7,
    HighPants = 8,
    Hands = 9,
    Outerwear = 10,
    FacialHair = 11,
    Mouth = 12,
    Eyes = 13,
    Hair = 14,
    Hood = 15,
    Back = 16,
    FaceAccessory = 17,
    Head = 18,
    Legs = 19,
    LeftLeg = 20,
    RightLeg = 21,
    Arms = 22,
    LeftArm = 23,
    RightArm = 24,
    Capes = 25,
    ClassicSkin = 26,
    Emote = 27,
    Unsupported = 28
};

class PersonaPieceData {
public:
    std::string mId;
    PersonaPieceType mPieceType = PersonaPieceType::Unknown;
    Uuid mPackId;
    bool mIsDefault = false;
    std::string mProductId;
};

class PersonaPieceTintData {
public:
    std::string mPieceType;
    int32_t mColor0Argb = 0;
    int32_t mColor1Argb = 0;
    int32_t mColor2Argb = 0;
    int32_t mColor3Argb = 0;
};

class SerializedSkin {
public:
    std::string mSkinId;
    std::string mPlayFabId;
    std::string mSkinResourcePatch;
    SkinImageData mSkinData;
    std::vector<SkinAnimationData> mAnimations;
    SkinImageData mCapeData;
    std::string mGeometryData;
    std::string mGeometryDataEngineVersion;
    std::string mAnimationData;
    std::string mCapeId;
    std::string mFullSkinId;
    bool mArmsWide = true;
    int32_t mColorArgb = 0;
    std::vector<PersonaPieceData> mPersonaPieces;
    std::vector<PersonaPieceTintData> mTintColors;
    bool mPremium = false;
    bool mPersona = false;
    bool mCapeOnClassic = false;
    bool mPrimaryUser = false;
    bool mOverridingPlayerAppearance = false;
    bool mTrusted = true;
    std::string mProfileHash;
};
