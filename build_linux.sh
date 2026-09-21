#!/bin/bash

set -e

BUILD_DIR="cmake-build-debug"
TOOLCHAIN_FILE="${BUILD_DIR}/build/Debug/generators/conan_toolchain.cmake"
# cppstd must match the conan dependency binaries (setting is part of the
# package_id); changing it triggers rebuild/re-download of all deps.
# Project code itself is always compiled with the C++20 set by
# CMAKE_CXX_STANDARD in the root CMakeLists.txt regardless of this value
CPPSTD=20
JOBS=8

function do_install() {
    echo "==== 1. Running Conan install ===="
    conan install . \
        --output-folder="${BUILD_DIR}" \
        --build=missing \
        -s build_type=Debug \
        -s compiler.cppstd=${CPPSTD}
}

function do_configure() {
    echo "==== 2. Running CMake configure ===="
    cmake -B "${BUILD_DIR}" -S . \
        -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
        -DCMAKE_BUILD_TYPE=Debug
}

function do_build() {
    echo "==== 3. Running CMake build ===="
    cmake --build "${BUILD_DIR}" -j${JOBS}
}

case "$1" in
    install)
        do_install
        ;;
    configure)
        do_configure
        ;;
    build)
        do_build
        ;;
    all)
        do_install
        do_configure
        do_build
        ;;
    *)
        echo "Usage: $0 {install|configure|build|all}"
        exit 1
        ;;
esac
