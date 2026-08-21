#include "Protocol/Packets/PlayerSkinPacket.h"

#include "Protocol/NetworkPacketHandler.h"

namespace {

    void writeImage(BinaryStream &stream, const SkinImageData &image) {
        stream.putLInt((uint32_t) image.mWidth);
        stream.putLInt((uint32_t) image.mHeight);
        stream.putByteArray(image.mData);
    }

    SkinImageData readImage(ReadOnlyBinaryStream &stream) {
        SkinImageData image;
        image.mWidth = (int32_t) stream.getLInt();
        image.mHeight = (int32_t) stream.getLInt();
        image.mData = stream.getByteArray();
        return image;
    }

    void writeAnimation(BinaryStream &stream, const SkinAnimationData &animation) {
        writeImage(stream, animation.mImage);
        stream.putUnsignedVarInt((uint32_t) animation.mTextureType);
        stream.putLFloat(animation.mFrames);
        stream.putUnsignedVarInt((uint32_t) animation.mExpressionType);
    }

    SkinAnimationData readAnimation(ReadOnlyBinaryStream &stream) {
        SkinAnimationData animation;
        animation.mImage = readImage(stream);
        animation.mTextureType = (AnimatedTextureType) stream.getUnsignedVarInt();
        animation.mFrames = stream.getLFloat();
        animation.mExpressionType = (AnimationExpressionType) stream.getUnsignedVarInt();
        return animation;
    }

    void writeSkin(BinaryStream &stream, const SerializedSkin &skin) {
        stream.putString(skin.mSkinId);
        stream.putString(skin.mPlayFabId);
        stream.putString(skin.mSkinResourcePatch);
        writeImage(stream, skin.mSkinData);

        stream.putUnsignedVarInt((uint32_t) skin.mAnimations.size());
        for (const SkinAnimationData &animation: skin.mAnimations) {
            writeAnimation(stream, animation);
        }

        writeImage(stream, skin.mCapeData);
        stream.putString(skin.mGeometryData);
        stream.putString(skin.mGeometryDataEngineVersion);
        stream.putString(skin.mAnimationData);
        stream.putString(skin.mCapeId);
        stream.putString(skin.mFullSkinId);

        stream.putByte(skin.mArmsWide ? 1 : 0);
        stream.putLInt((uint32_t) skin.mColorArgb);

        stream.putUnsignedVarInt((uint32_t) skin.mPersonaPieces.size());
        for (const PersonaPieceData &piece: skin.mPersonaPieces) {
            stream.putString(piece.mId);
            stream.putLInt((uint32_t) piece.mPieceType);
            stream.putUuid(piece.mPackId);
            stream.putBool(piece.mIsDefault);
            stream.putString(piece.mProductId);
        }

        stream.putUnsignedVarInt((uint32_t) skin.mTintColors.size());
        for (const PersonaPieceTintData &tint: skin.mTintColors) {
            stream.putString(tint.mPieceType);
            stream.putLInt((uint32_t) tint.mColor0Argb);
            stream.putLInt((uint32_t) tint.mColor1Argb);
            stream.putLInt((uint32_t) tint.mColor2Argb);
            stream.putLInt((uint32_t) tint.mColor3Argb);
        }

        stream.putBool(skin.mPremium);
        stream.putBool(skin.mPersona);
        stream.putBool(skin.mCapeOnClassic);
        stream.putBool(skin.mPrimaryUser);
        stream.putBool(skin.mOverridingPlayerAppearance);

        stream.putString(skin.mTrusted ? "true" : "false");
        stream.putString(skin.mProfileHash);
    }

    SerializedSkin readSkin(ReadOnlyBinaryStream &stream) {
        SerializedSkin skin;
        skin.mSkinId = stream.getString();
        skin.mPlayFabId = stream.getString();
        skin.mSkinResourcePatch = stream.getString();
        skin.mSkinData = readImage(stream);

        uint32_t animationCount = stream.getUnsignedVarInt();
        skin.mAnimations.reserve(animationCount);
        for (uint32_t i = 0; i < animationCount; i++) {
            skin.mAnimations.push_back(readAnimation(stream));
        }

        skin.mCapeData = readImage(stream);
        skin.mGeometryData = stream.getString();
        skin.mGeometryDataEngineVersion = stream.getString();
        skin.mAnimationData = stream.getString();
        skin.mCapeId = stream.getString();
        skin.mFullSkinId = stream.getString();

        skin.mArmsWide = stream.getByte() == 1;
        skin.mColorArgb = (int32_t) stream.getLInt();

        uint32_t pieceCount = stream.getUnsignedVarInt();
        skin.mPersonaPieces.reserve(pieceCount);
        for (uint32_t i = 0; i < pieceCount; i++) {
            PersonaPieceData piece;
            piece.mId = stream.getString();
            piece.mPieceType = (PersonaPieceType) stream.getLInt();
            piece.mPackId = stream.getUuid();
            piece.mIsDefault = stream.getBool();
            piece.mProductId = stream.getString();
            skin.mPersonaPieces.push_back(piece);
        }

        uint32_t tintCount = stream.getUnsignedVarInt();
        skin.mTintColors.reserve(tintCount);
        for (uint32_t i = 0; i < tintCount; i++) {
            PersonaPieceTintData tint;
            tint.mPieceType = stream.getString();
            tint.mColor0Argb = (int32_t) stream.getLInt();
            tint.mColor1Argb = (int32_t) stream.getLInt();
            tint.mColor2Argb = (int32_t) stream.getLInt();
            tint.mColor3Argb = (int32_t) stream.getLInt();
            skin.mTintColors.push_back(tint);
        }

        skin.mPremium = stream.getBool();
        skin.mPersona = stream.getBool();
        skin.mCapeOnClassic = stream.getBool();
        skin.mPrimaryUser = stream.getBool();
        skin.mOverridingPlayerAppearance = stream.getBool();

        skin.mTrusted = stream.getString() == "true";
        skin.mProfileHash = stream.getString();

        return skin;
    }

}

PlayerSkinPacket::PlayerSkinPacket() = default;

void PlayerSkinPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putUuid(mUuid);
    writeSkin(stream, mSkin);
    stream.putString(mNewSkinName);
    stream.putString(mOldSkinName);
    stream.putBool(mTrustedSkin);
}

void PlayerSkinPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    mUuid = stream.getUuid();
    mSkin = readSkin(stream);
    mNewSkinName = stream.getString();
    mOldSkinName = stream.getString();

    if (!stream.feof()) {
        mTrustedSkin = stream.getBool();
    }
}

void PlayerSkinPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
