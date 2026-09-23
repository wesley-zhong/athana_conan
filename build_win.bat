@echo off
rem Windows 构建脚本，与 build_linux.sh 对应
rem 用法: build_win.bat {install|configure|build|all}

setlocal

set "BUILD_DIR=cmake-build-debug"
set "TOOLCHAIN_FILE=%BUILD_DIR%\build\generators\conan_toolchain.cmake"
rem cppstd 必须与 conan 依赖二进制一致（是 package_id 的一部分），
rem 改动会触发全部依赖重新构建/下载
set "CPPSTD=20"
set "JOBS=28"

rem VS 环境（与 conan_toolchain.cmake 的 v143 toolset 对应）
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" (
    echo vcvars not found: %VCVARS%
    exit /b 1
)
call "%VCVARS%" >nul || exit /b 1

rem ninja 不随 vcvars 提供，优先用 CLion 自带的
set "NINJA_ARG="
set "NINJA_EXE=C:\Program Files\JetBrains\CLion 2026.2.1\bin\ninja\win\x64\ninja.exe"
if exist "%NINJA_EXE%" set NINJA_ARG=-DCMAKE_MAKE_PROGRAM="%NINJA_EXE%"

if /i "%1"=="install"  (call :do_install   || exit /b 1) & goto :eof
if /i "%1"=="configure" (call :do_configure || exit /b 1) & goto :eof
if /i "%1"=="build"    (call :do_build     || exit /b 1) & goto :eof
if /i "%1"=="all"      (call :do_install   || exit /b 1) & (call :do_configure || exit /b 1) & call :do_build & goto :eof

echo Usage: %0 {install^|configure^|build^|all}
exit /b 1

:do_install
echo ==== 1. Running Conan install ====
conan install . --output-folder="%BUILD_DIR%" --build=missing -s build_type=Debug -s compiler.cppstd=%CPPSTD%
exit /b %errorlevel%

:do_configure
echo ==== 2. Running CMake configure ====
cmake -B "%BUILD_DIR%" -S . -G Ninja %NINJA_ARG% ^
    -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN_FILE%" ^
    -DCMAKE_BUILD_TYPE=Debug
exit /b %errorlevel%

:do_build
echo ==== 3. Running CMake build ====
cmake --build "%BUILD_DIR%" -j%JOBS%
exit /b %errorlevel%
