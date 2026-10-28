import os

from conan import ConanFile
from conan.tools.cmake import cmake_layout
from conan.tools.files import copy
from conan.tools.system.package_manager import Apt


class ImGuiExample(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    default_options = {
        "grpc/*:cpp_plugin": True,
        "grpc/*:csharp_ext": False,
        "grpc/*:csharp_plugin": False,
        "grpc/*:node_plugin": False,
        "grpc/*:objective_c_plugin": False,
        "grpc/*:php_plugin": False,
        "grpc/*:python_plugin": False,
        "grpc/*:ruby_plugin": False,
        "grpc/*:otel_plugin": False,
    }

    def requirements(self):
        self.requires("spdlog/1.17.0")
        self.requires("grpc/1.84.0")


    def layout(self):
        cmake_layout(self)