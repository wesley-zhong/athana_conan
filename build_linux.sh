conan install . --output-folder=cmake-build-debug --build=missing -s build_type=Debug -s compiler.cppstd=17
cmake -B  cmake-build-debug -S . -DCMAKE_TOOLCHAIN_FILE=cmake-build-debug/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug -j8