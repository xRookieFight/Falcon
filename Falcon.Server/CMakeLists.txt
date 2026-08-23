cmake_minimum_required(VERSION 3.16)
project(Falcon.Server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FALCON_RECIPES_JSON "${CMAKE_CURRENT_SOURCE_DIR}/resources/recipes.json")
set(FALCON_ITEM_TAGS_JSON "${CMAKE_CURRENT_SOURCE_DIR}/resources/item_tags.json")
if(NOT EXISTS "${FALCON_RECIPES_JSON}" OR NOT EXISTS "${FALCON_ITEM_TAGS_JSON}")
    message(FATAL_ERROR "Falcon crafting data files are missing")
endif()

file(READ "${FALCON_RECIPES_JSON}" FALCON_RECIPES_JSON_CONTENT)
file(READ "${FALCON_ITEM_TAGS_JSON}" FALCON_ITEM_TAGS_JSON_CONTENT)
set(FALCON_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${FALCON_GENERATED_DIR}")
configure_file(CraftingRecipeJson.h.in "${FALCON_GENERATED_DIR}/CraftingRecipeJson.h" @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${FALCON_RECIPES_JSON}" "${FALCON_ITEM_TAGS_JSON}")

add_executable(FalconServer
        src/Server.cpp
        src/Core/Debug/BedrockLog.cpp
        src/Core/Debug/ContentLogEndPoint.cpp
        src/Core/Debug/FileLogEndPoint.cpp
        src/Core/Debug/LogAreaID.cpp
        src/Core/Debug/LogDetails.cpp
        src/Core/Debug/LogLevel.cpp
        src/Network/BatchedNetworkPeer.cpp
        src/Network/CompressedNetworkPeer.cpp
        src/Network/NetworkEnums.cpp
        src/Network/NetworkIdentifier.cpp
        src/Network/NetworkHandler.cpp
        src/Network/PingedCompatibleServer.cpp
        src/Network/RakNetInstance.cpp
        src/Network/RakNetRemotePeer.cpp
        src/Network/RakNetServerLocator.cpp
        src/Network/RakPeerHelper.cpp
        src/Network/ServerNetworkHandler.cpp
        src/Network/ConnectionRequest.cpp
        src/Network/LoginChainVerifier.cpp
        src/Network/AuthKeyProvider.cpp
        src/Network/BadPacketCheck.cpp
        src/Network/BreakDebug.cpp
        src/Network/BlockActionHandler.cpp
        src/Network/ChatHandler.cpp
        src/Network/InventoryHandler.cpp
        src/Network/LoginHandler.cpp
        src/Network/MovementHandler.cpp
        src/Network/ItemActorHandler.cpp
        src/Command/Command.cpp
        src/Command/ConsoleCommandSender.cpp
        src/Command/DeopCommand.cpp
        src/Command/EnchantCommand.cpp
        src/Command/EffectCommand.cpp
        src/Command/OpCommand.cpp
        src/Command/CommandMap.cpp
        src/Command/GameModeCommand.cpp
        src/Command/GiveCommand.cpp
        src/Command/KillCommand.cpp
        src/Command/PlayerCommandSender.cpp
        src/Block/BlockData.cpp
        src/Block/CreativeContentTable.cpp
        src/Block/VanillaBlocks.cpp
        src/Actor/Actor.cpp
        src/Actor/ActorAttributes.cpp
        src/Actor/ActorFlags.cpp
        src/Actor/MobEffect.cpp
        src/Actor/ExperienceManager.cpp
        src/Actor/ItemActor.cpp
        src/Actor/ServerPlayer.cpp
        src/Item/CraftingRecipeTable.cpp
        src/Item/EnchantmentTable.cpp
        src/Item/ItemDataTable.cpp
        src/Item/ItemEnchantments.cpp
        src/Item/ItemNetworkIdTable.cpp
        src/Item/StringToItemParser.cpp
        src/Item/VanillaItems.cpp
        src/Inventory/InventoryManager.cpp
        src/Inventory/ItemStackNbt.cpp
        src/Inventory/ItemStackRequestHandler.cpp
        src/Inventory/PlayerInventory.cpp
        src/Level/FlatChunkGenerator.cpp
        src/Level/BiomeRegistry.cpp
        src/Level/Chunk.cpp
        src/Level/LevelStorage.cpp
        src/Level/SubChunk.cpp
        src/Level/Level.cpp
        src/Player/PlayerDataProvider.cpp
        src/Server/OpList.cpp
        src/Server/PropertiesSettings.cpp
        src/Server/ResourcePackManager.cpp
)

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

target_include_directories(FalconServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${FALCON_GENERATED_DIR} ${ZLIB_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR})
target_link_libraries(FalconServer PRIVATE FalconNetwork FalconProtocol leveldb)

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(FalconServer PRIVATE -static -fuse-ld=lld)
    target_link_libraries(FalconServer PRIVATE z ssl crypto ws2_32 crypt32)
else()
    target_link_libraries(FalconServer PRIVATE ZLIB::ZLIB OpenSSL::SSL OpenSSL::Crypto)
endif()
