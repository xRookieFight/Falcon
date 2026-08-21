cmake_minimum_required(VERSION 3.16)
project(Falcon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(Falcon.Network)
add_subdirectory(Falcon.Protocol)
add_subdirectory(Falcon.Server)
