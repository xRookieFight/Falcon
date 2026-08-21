@echo off
setlocal

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] cmake not found in PATH.
    exit /b 1
)

where g++ >nul 2>nul
if errorlevel 1 (
    echo [ERROR] g++ not found in PATH. Install MSYS2 UCRT64 and add it to PATH.
    exit /b 1
)

where ninja >nul 2>nul
if errorlevel 1 (
    echo [ERROR] ninja not found in PATH. Install MSYS2 UCRT64 and add it to PATH.
    exit /b 1
)

echo [1/3] Removing previous build directory...
if exist build (
    rmdir /s /q build
)

echo [2/3] Configuring project with CMake...
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++ -DCMAKE_MAKE_PROGRAM=ninja
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

echo [3/3] Building FalconServer...
cmake --build build --config Release
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

if not exist build\Falcon.Server\FalconServer.exe (
    echo [ERROR] Build reported success but FalconServer.exe was not produced.
    exit /b 1
)

echo.
echo Build succeeded: build\Falcon.Server\FalconServer.exe
endlocal
