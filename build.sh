#!/usr/bin/env bash
set -uo pipefail

LOG=build.txt
: > "$LOG"

for tool in cmake g++ ninja; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "[ERROR] $tool not found in PATH."
        exit 1
    fi
done

echo "[2/3] Configuring project with CMake..."
echo "      First run fetches leveldb, quickjs and libdatachannel - this can take a while."
if ! cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++ -DCMAKE_MAKE_PROGRAM=ninja 2>&1 | tee -a "$LOG"; then
    echo "[ERROR] CMake configuration failed. See $LOG for details."
    exit 1
fi

echo "[3/3] Building FalconServer..."
cmake --build build --config Release 2>&1 | tee -a "$LOG"
BUILD_RESULT=${PIPESTATUS[0]}

if [ "$BUILD_RESULT" -ne 0 ]; then
    echo "[ERROR] Build failed. Full output saved to $LOG."
    exit 1
fi

if [ ! -f build/Falcon.Server/FalconServer ]; then
    echo "[ERROR] Build reported success but FalconServer was not produced. See $LOG."
    exit 1
fi

echo
echo "Build succeeded: build/Falcon.Server/FalconServer"
echo "Full build log saved to $LOG"
