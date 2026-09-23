# athena

 conan install .  --output-folder=cmake-build-debug  --build=missing  -s build_type=Debug -s compiler.cppstd=20


Settings → Build, Execution, Deployment → CMake → 选中 "Debug" profile,把 Toolchain file 设为：
cmake-build-debug/build/generators/conan_toolchain.cmake
或者直接启用 conan-default preset profile(Workspace 里显示 ENABLED=false),然后 Reload CMake