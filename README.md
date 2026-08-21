<p align="center">
	<b>Falcon</b>
	<br>
	<b>Minecraft: Bedrock Edition server software written from scratch in C++</b>
	<br>
	Not affiliated with Mojang AB.
</p>

<p align="center">
	<img src="https://img.shields.io/badge/minecraft-v1.26.40%20(Bedrock)-56383E" alt="Minecraft">
	<img src="https://img.shields.io/badge/protocol-2168-blue" alt="Protocol">
	<img src="https://img.shields.io/badge/language-C%2B%2B17-00599C" alt="C++17">
	<img src="https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey" alt="Platform">
</p>

## What is this?

Falcon is a Minecraft: Bedrock Edition server built from the ground up in C++17. There is no game
engine and no third-party networking library underneath it — RakNet, the Bedrock protocol and the
server logic are all implemented in this repository.

- ⚡ **Native C++17** — no runtime to install, the server ships as a single self-contained executable
- 🧱 **Own RakNet implementation** — sockets, reliability layer and connection handshake written here
- 📦 **230+ protocol packets** — encoded and decoded against the current Bedrock wire format
- 🗜️ **Batching and compression** — zlib game-packet pipeline, as the vanilla client expects
- 🌍 **Flat world generation** — sub-chunk encoding with hashed block runtime IDs
- 🧩 **Layered architecture** — network, protocol and server concerns stay separate

## Building

Falcon needs CMake 3.16+, a C++17 compiler and zlib. On Windows the reference toolchain is MSYS2
UCRT64.

```
cmake -B build -G Ninja
cmake --build build
```

On Windows, `build.bat` runs a full clean rebuild in one step. The resulting binary,
`build/Falcon.Server/FalconServer.exe`, is statically linked and runs without any accompanying
runtime DLLs.

## Licensing information

Falcon is not affiliated with Mojang. All brands and trademarks belong to their respective owners.
Falcon is not a Mojang-approved software, nor is it associated with Mojang.
