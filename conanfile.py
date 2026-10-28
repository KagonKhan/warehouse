import os

from conan import ConanFile
from conan.tools.cmake import cmake_layout
from conan.tools.files import copy
from conan.tools.system.package_manager import Apt


class ImGuiExample(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"


    def requirements(self):
        self.requires("spdlog/1.17.0")
        
        
    def system_requirements(self):
        packages = [
            "pkg-config"
        ]

        Apt(self).install(packages)


    def layout(self):
        cmake_layout(self)