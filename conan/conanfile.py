from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
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
    short_paths = True
    options = {
        "shared": [True, False],
        "build_tests": [True, False],
    }
    default_options = {
        "shared": False,
        "build_tests": False,
        "glad:gl_version": "4.6", # 4.2 is required for glBindImageTexture()
                                  # 4.6 notably for FRAGMENT_SHADER_INVOCATIONS query
        # Note: macos only provides GL_ARB_texture_storage and GL_ARB_internalformat_query
        "glad:extensions": ("GL_KHR_debug,"
            "GL_ARB_texture_storage,"
            "GL_ARB_clear_texture,"
            "GL_ARB_program_interface_query,"
            "GL_ARB_shader_storage_buffer_object,"
            "GL_ARB_base_instance,"
            "GL_ARB_multi_draw_indirect,"
            "GL_ARB_texture_filter_anisotropic," # anisotropic texture filtering
        )
    }

    requires = (
        ("freetype/2.12.1"),
        ("glad/0.1.36"),
        ("glfw/3.3.8@adnn/patch"),
        ("nlohmann_json/3.11.2"),
        ("spdlog/1.13.0"),
        ("utfcpp/4.0.1"),
        ("imgui/1.89.8"),

        ("handy/e2b164a804@adnn/develop"),
        ("math/8c49b882e7@adnn/develop"),
    )

    build_policy = "missing"
    generators = "CMakeDeps", "CMakeToolchain"
    keep_imports = True


    python_requires="shred_conan_base/0.0.5@adnn/stable"
    python_requires_extend="shred_conan_base.ShredBaseConanFile"


    def imports(self):
        # see: https://blog.conan.io/2019/06/26/An-introduction-to-the-Dear-ImGui-library.html
        # the imgui package is designed this way: consumer has to import desired backends.
        self.copy("imgui_impl_glfw.cpp",         src="./res/bindings", dst=path.join(self.folders.build, "conan_imports/imgui_backends"))
        self.copy("imgui_impl_opengl3.cpp",      src="./res/bindings", dst=path.join(self.folders.build, "conan_imports/imgui_backends"))
        self.copy("imgui_impl_glfw.h",           src="./res/bindings", dst=path.join(self.folders.build, "conan_imports/imgui_backends"))
        self.copy("imgui_impl_opengl3.h",        src="./res/bindings", dst=path.join(self.folders.build, "conan_imports/imgui_backends"))
        self.copy("imgui_impl_opengl3_loader.h", src="./res/bindings", dst=path.join(self.folders.build, "conan_imports/imgui_backends"))
