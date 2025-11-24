from conan import ConanFile

class AthenaDeps(ConanFile):
    name = "athena_deps"
    version = "0.1"
    # 使用 MSVC / Visual Studio → 必须声明 settings（但不包含 build_type）
    settings = "os", "arch", "compiler", "build_type"

    generators = "CMakeDeps", "CMakeToolchain"

    requires = [
        "mongo-cxx-driver/3.6.7",
        "libmysqlclient/8.1.0",
        "hiredis/1.3.0",
        "protobuf/5.29.3",
        "libuv/1.51.0",
        "spdlog/1.16.0",
        "sol2/3.5.0"
    ]
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        # 强制所有依赖使用 zstd/1.5.7
        self.requires("zstd/1.5.7", override=True)
