from conan import ConanFile
from conan.tools.cmake import cmake_layout

class AthenaDeps(ConanFile):
    name = "athena_deps"
    version = "0.1"
    # 使用 MSVC / Visual Studio → 必须声明 settings（但不包含 build_type）
    settings = "os", "arch", "compiler", "build_type"

    generators = ("CMakeDeps", "CMakeToolchain")

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("mongo-cxx-driver/3.6.7")
        self.requires("libmysqlclient/8.1.0")
        self.requires("hiredis/1.3.0")
        self.requires("libuv/1.51.0")
        self.requires("spdlog/1.16.0")
        self.requires("sol2/3.5.0")
        self.requires("tomlplusplus/3.4.0")
        self.requires("etcd-cpp-apiv3/0.15.4")
        self.requires("grpc/1.54.3")

        # 强制统一版本
        self.requires("zstd/1.5.7", override=True)
        self.requires("protobuf/3.21.12", override=True)
        self.requires("boost/1.83.0", override=True)

