cmake_minimum_required(VERSION 3.16)
project(Falcon LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(LEVELDB_INSTALL OFF CACHE BOOL "" FORCE)
set(LEVELDB_SNAPPY OFF CACHE BOOL "" FORCE)
set(LEVELDB_ZSTD OFF CACHE BOOL "" FORCE)
set(LEVELDB_TCMALLOC OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/leveldb)

add_subdirectory(Falcon.Network)
add_subdirectory(Falcon.Protocol)
add_subdirectory(Falcon.Server)
