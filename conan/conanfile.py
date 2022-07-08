from conans import ConanFile, tools
from conan.tools.cmake import CMake
from conan.tools.files import copy

from os import path


class GraphicsConan(ConanFile):
    name = "graphics"
    license = "MIT"
    author = "adnn"
    url = "https://github.com/Adnn/graphics"
    description = "Graphics rendering generic library, both software and with OpenGL"
    topics = ("opengl", "graphics", "2D", "3D")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "build_tests": [True, False],
    }
    default_options = {
        "shared": False,
        "build_tests": False,
        "glad:gl_version": "4.1",
        # Note: macos only provides GL_ARB_texture_storage and GL_ARB_internalformat_query
        "glad:extensions": "GL_KHR_debug, GL_ARB_texture_storage, GL_ARB_clear_texture",
    }

    requires = (
        ("boost/1.79.0"),
        ("freetype/2.12.1"),
        ("glad/0.1.34"),
        ("glfw/3.3.6"),
        ("nlohmann_json/3.9.1"),
        ("spdlog/1.9.2"),
        ("utfcpp/3.2.1"),
        ("zlib/1.2.12"),
        ("imgui/1.87"),

        ("handy/4ecfa5b125@adnn/develop"),
        ("math/f140b4368f@adnn/develop"),
    )

    build_policy = "missing"
    generators = "CMakeDeps", "CMakeToolchain"

    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto",
        "submodule": "recursive",
    }

    python_requires="shred_conan_base/0.0.2@adnn/develop"
    python_requires_extend="shred_conan_base.ShredBaseConanFile"


    def _generate_cmake_configfile(self):
        """ Generates a conanuser_config.cmake file which includes the file generated by """
        """ cmake_paths generator, and forward the remaining options to CMake. """
        with open("conanuser_config.cmake", "w") as config:
            config.write("message(STATUS \"Including user generated conan config.\")\n")
            # avoid path.join, on Windows it outputs '\', which is a string escape sequence.
            config.write("set({} {})\n".format("BUILD_tests", self.options.build_tests))
            config.write("set(CMAKE_EXPORT_COMPILE_COMMANDS 1)\n")


    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure()
        return cmake


    def configure(self):
        tools.check_min_cppstd(self, "17")


    def generate(self):
           self._generate_cmake_configfile()


    def imports(self):
        # see: https://blog.conan.io/2019/06/26/An-introduction-to-the-Dear-ImGui-library.html
        # the imgui package is designed this way: consumer has to import desired backends.
        self.copy("imgui_impl_glfw.cpp",         src="./res/bindings", dst=path.join(self.folders.build, "conan_imports/imgui_backends"))
        self.copy("imgui_impl_opengl3.cpp",      src="./res/bindings", dst=path.join(self.folders.build, "conan_imports/imgui_backends"))
        self.copy("imgui_impl_glfw.h",           src="./res/bindings", dst=path.join(self.folders.build, "conan_imports/imgui_backends"))
        self.copy("imgui_impl_opengl3.h",        src="./res/bindings", dst=path.join(self.folders.build, "conan_imports/imgui_backends"))
        self.copy("imgui_impl_opengl3_loader.h", src="./res/bindings", dst=path.join(self.folders.build, "conan_imports/imgui_backends"))


    def build(self):
        cmake = self._configure_cmake()
        cmake.build()


    def package(self):
        cmake = self._configure_cmake()
        cmake.install()


    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "none")
        if self.folders.build_folder:
            self.cpp_info.builddirs.append(self.folders.build_folder)
        else:
            self.cpp_info.builddirs.append(path.join('lib', 'cmake'))
