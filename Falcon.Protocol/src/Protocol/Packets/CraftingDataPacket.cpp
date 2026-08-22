#include "Protocol/Packets/CraftingDataPacket.h"

#include "Protocol/NetworkPacketHandler.h"

CraftingDataPacket::CraftingDataPacket() = default;

namespace {
    void writeIngredient(BinaryStream &stream, const RecipeIngredientEntry &ingredient) {
        if (!ingredient.mHasItem) {
            stream.putUnsignedVarInt(0);
            stream.putVarInt(0x7fff);
            stream.putVarInt(ingredient.mCount);
            return;
        }

        stream.putUnsignedVarInt(1);
        stream.putString("name");
        stream.putString(ingredient.mItemId);
        stream.putVarInt(ingredient.mAuxValue);
        stream.putVarInt(ingredient.mCount);
    }

    void writeOutput(BinaryStream &stream, const RecipeOutputEntry &output) {
        stream.putVarInt(output.mRuntimeId);
        stream.putLShort((uint16_t) output.mCount);
        stream.putUnsignedVarInt((uint32_t) output.mMeta);
        stream.putVarInt(-1);
        stream.putString("");
    }

    void writeRecipe(BinaryStream &stream, const CraftingRecipeEntry &recipe, bool shaped) {
        stream.putString(recipe.mRecipeId);

        if (shaped) {
            stream.putVarInt(recipe.mWidth);
            stream.putVarInt(recipe.mHeight);
        }

        stream.putUnsignedVarInt((uint32_t) recipe.mInputs.size());
        for (const RecipeIngredientEntry &ingredient: recipe.mInputs)
            writeIngredient(stream, ingredient);

        stream.putUnsignedVarInt((uint32_t) recipe.mOutputs.size());
        for (const RecipeOutputEntry &output: recipe.mOutputs)
            writeOutput(stream, output);

        stream.putUuid(recipe.mUuid);
        stream.putString(recipe.mBlockName);
        stream.putVarInt(recipe.mPriority);

        if (shaped)
            stream.putBool(recipe.mSymmetric);

        stream.putBool(false);
        stream.putVarInt(recipe.mRecipeNetId);
    }
}

void CraftingDataPacket::write(BinaryStream &stream, const PacketCodecContext &context) const {
    stream.putUnsignedVarInt((uint32_t) mShapedRecipes.size());
    for (const CraftingRecipeEntry &recipe: mShapedRecipes)
        writeRecipe(stream, recipe, true);

    stream.putUnsignedVarInt((uint32_t) mShapelessRecipes.size());
    for (const CraftingRecipeEntry &recipe: mShapelessRecipes)
        writeRecipe(stream, recipe, false);

    for (int i = 0; i < 9; i++)
        stream.putArrayLength(0);

    stream.putBool(mCleanRecipes);
}

void CraftingDataPacket::read(ReadOnlyBinaryStream &stream, const PacketCodecContext &context) {
    for (int i = 0; i < 11; i++) {
        uint32_t length = stream.getArrayLength();
        for (uint32_t j = 0; j < length; j++) {
        }
    }
    mCleanRecipes = stream.getBool();
}

void CraftingDataPacket::handle(const NetworkIdentifier &id, NetworkPacketHandler &handler) const {
    handler.handle(id, *this);
}
