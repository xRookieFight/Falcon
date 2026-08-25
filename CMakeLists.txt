cmake_minimum_required(VERSION 3.16)
project(Falcon LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(LEVELDB_INSTALL OFF CACHE BOOL "" FORCE)
set(LEVELDB_SNAPPY OFF CACHE BOOL "" FORCE)
set(LEVELDB_ZSTD OFF CACHE BOOL "" FORCE)
set(LEVELDB_TCMALLOC OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    leveldb
    GIT_REPOSITORY https://github.com/NaySurGithub/leveldb.git
    GIT_TAG dc68a4cac5f2ece6cb990ef0c8e69c677b2dcba7
)
FetchContent_MakeAvailable(leveldb)

set(QJS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QJS_BUILD_LIBC OFF CACHE BOOL "" FORCE)
set(QJS_ENABLE_ASAN OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    quickjs
    GIT_REPOSITORY https://github.com/quickjs-ng/quickjs.git
    GIT_TAG v0.10.1
)
FetchContent_MakeAvailable(quickjs)

add_subdirectory(Falcon.Network)
add_subdirectory(Falcon.Protocol)
add_subdirectory(Falcon.Server)
