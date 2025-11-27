#!/bin/bash

set -e

BUILD_DIR="cmake-build-debug"
TOOLCHAIN_FILE="${BUILD_DIR}/conan_toolchain.cmake"
CPPSTD=17
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
