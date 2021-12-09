from conans import ConanFile, tools
from conan.tools.cmake import CMake

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
        "glad:extensions": "GL_KHR_debug, GL_ARB_texture_storage",
    }

    requires = (
        ("boost/1.77.0"),
        ("freetype/2.11.0"),
        ("glad/0.1.34"),
        ("glfw/3.3.4"),
        ("nlohmann_json/3.9.1"),
        ("spdlog/1.9.2"),
        ("utfcpp/3.2.1"),

        ("math/ead49a29d9@adnn/develop"),
    )

    build_requires = ("cmake/3.20.4",)

    build_policy = "missing"
    generators = "cmake_paths", "cmake_find_package", "CMakeToolchain"

    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto",
        "submodule": "recursive",
    }


    def _generate_cmake_configfile(self):
        """ Generates a conanuser_config.cmake file which includes the file generated by """
        """ cmake_paths generator, and forward the remaining options to CMake. """
        with open("conanuser_config.cmake", "w") as config:
            config.write("message(STATUS \"Including user generated conan config.\")\n")
            # avoid path.join, on Windows it outputs '\', which is a string escape sequence.
            config.write("include(\"{}\")\n".format("${CMAKE_CURRENT_LIST_DIR}/conan_paths.cmake"))
            config.write("set({} {})\n".format("BUILD_tests", self.options.build_tests))


    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure()
        return cmake


    def configure(self):
        tools.check_min_cppstd(self, "17")


    def generate(self):
           self._generate_cmake_configfile()


    def build(self):
        cmake = self._configure_cmake()
        cmake.build()


    def package(self):
        cmake = self._configure_cmake()
        cmake.install()


    def package_info(self):
        pass
