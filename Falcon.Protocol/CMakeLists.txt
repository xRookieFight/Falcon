cmake_minimum_required(VERSION 3.16)
project(Falcon.Protocol LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE FALCON_PROTOCOL_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(FalconProtocol STATIC ${FALCON_PROTOCOL_SOURCES})

target_include_directories(FalconProtocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
