<p align="center">
	<b>Falcon</b>
	<br>
	<b>A Minecraft: Bedrock Edition server software written from scratch in modern C++</b>
	<br>
	Not affiliated with Mojang AB.
</p>

<p align="center">
	<img src="https://img.shields.io/badge/minecraft-v1.26.40%20(Bedrock)-56383E" alt="Minecraft">
	<img src="https://img.shields.io/badge/protocol-2168-blue" alt="Protocol">
	<img src="https://img.shields.io/badge/language-C%2B%2B17-00599C" alt="C++17">
	<img src="https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey" alt="Platform">
	<img src="https://img.shields.io/badge/status-in%20development-orange" alt="Status">
</p>

## What is this?

Falcon is a Minecraft: Bedrock Edition server written from the ground up in C++17, with no game
engine and no third-party networking library. RakNet, the Bedrock protocol and the server logic are
all implemented in this repository.

- ⚡ **Native C++17** — no runtime to install, the server ships as a single self-contained executable
- 🧱 **Own RakNet implementation** — sockets, reliability layer and connection handshake written here
- 📦 **176 protocol packets** — encoded and decoded against the current Bedrock wire format
- 🗜️ **Batching and compression** — zlib game-packet pipeline, as the vanilla client expects
- 🌍 **Flat world generation** — sub-chunk encoding with hashed block runtime IDs
- 🧩 **Layered architecture** — network, protocol and server concerns stay separate

## Project layout

Falcon is split into libraries that depend on each other in one direction only.

| Module | Role |
|---|---|
| `Falcon.Network` | RakNet transport: UDP sockets, reliability layer, connection handshake |
| `Falcon.Protocol` | Bedrock packet definitions, NBT, binary streams, block and item registries |
| `Falcon.Server` | Server logic: players, login sequence, world, entities, configuration |

`Falcon.Server` depends on both lower layers; `Falcon.Protocol` and `Falcon.Network` never depend on
the server.

## Building

Falcon needs CMake 3.16+, a C++17 compiler and zlib. On Windows the reference toolchain is MSYS2
UCRT64.

```
cmake -B build -G Ninja
cmake --build build
```

The resulting binary is `build/Falcon.Server/FalconServer.exe`. It is statically linked, so it runs
without any accompanying runtime DLLs.

## Running

Run the executable. On first start it falls back to built-in defaults and writes its log to
`FalconServer.log`.

```
./FalconServer
```

Server settings are read from `server.properties` in the working directory when present. The server
listens on UDP port `19132` by default, dual-stack, so it is reachable over both IPv4 and IPv6.

## Licensing information

Falcon is not affiliated with Mojang. All brands and trademarks belong to their respective owners.
Falcon is not a Mojang-approved software, nor is it associated with Mojang.
