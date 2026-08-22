#include "Block/VanillaBlocks.h"

#include "Block/BlockTypeIds.h"

Block VanillaBlocks::AIR() {
    return Block(BlockTypeIds::AIR, "minecraft:air", "Air");
}

Block VanillaBlocks::BEDROCK() {
    return Block(BlockTypeIds::BEDROCK, "minecraft:bedrock", "Bedrock");
}

Block VanillaBlocks::GRASS() {
    return Block(BlockTypeIds::GRASS, "minecraft:grass_block", "Grass Block");
}

Block VanillaBlocks::STONE() {
    return Block(BlockTypeIds::STONE, "minecraft:stone", "Stone");
}

std::vector<Block> VanillaBlocks::getAll() {
    std::vector<Block> blocks;
    blocks.push_back(AIR());
    blocks.push_back(BEDROCK());
    blocks.push_back(GRASS());
    blocks.push_back(STONE());
    return blocks;
}

const Block *VanillaBlocks::fromIdentifier(const std::string &identifier) {
    static const std::vector<Block> blocks = getAll();

    for (const Block &block: blocks) {
        if (block.getIdentifier() == identifier)
            return &block;
    }

    return nullptr;
}
