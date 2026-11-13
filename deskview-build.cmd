@echo off
rem Build Deskflow + DeskView on Windows (same settings as Deskflow's CI).
rem Needs: Visual Studio 2022 Build Tools (C++), Qt 6.11 msvc2022_64 in C:\Qt, vcpkg with openssl:x64-windows-release.
rem Usage:  deskview-build.cmd [configure]     (configure = re-run CMake from scratch)
setlocal
if not defined QT_DIR set QT_DIR=C:\Qt\6.11.3\msvc2022_64
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
rem vcvars points VCPKG_ROOT at Visual Studio's bundled vcpkg; use ours (with openssl built) instead.
if not defined DV_VCPKG set DV_VCPKG=%~dp0..\vcpkg
cd /d "%~dp0"
if /i "%1"=="configure" if exist build rmdir /s /q build
if not exist build\build.ninja (
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSKIP_BUILD_TESTS=ON -DBUILD_INSTALLER=OFF ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_TOOLCHAIN_FILE="%DV_VCPKG%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-release ^
    -DVCPKG_MANIFEST_MODE=OFF || exit /b 1
)
cmake --build build -j %NUMBER_OF_PROCESSORS% || exit /b 1
echo BUILD OK
