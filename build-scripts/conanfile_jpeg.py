from conan import ConanFile
from conan.tools.files import (
    copy,
)
import os


required_conan_version = ">=1.53.0"


class JPEGConan(ConanFile):
    name = "add_jpeg"
    description = ""
    settings = "os", "arch", "compiler", "build_type"
    options = {
    }
    default_options = {
    }

    def configure(self):
        self.options["libjpeg"].shared = True

    def requirements(self):
        self.requires("libjpeg/9e")
    
    def generate(self):
        for dep in self.dependencies.values():
            if dep.package_folder:
                print(f"copying {dep}: {dep.package_folder} -> {self.build_folder}")
                if self.settings.os == "Windows":
                    # Copy all the libraries to lib3rdparty
                    # Cannot only grab specific types, because for some reason
                    # tensorflow-gpu has c++ headers with no extension
                    copy(self, "*", src=dep.package_folder, dst=os.path.join("lib3rdparty", str(dep.ref).split('/')[0]))
                    # Copy DLLs and Crashpad executables to bin folder
                    # copy(self, "*.xml", src=os.path.join(dep.package_folder, "bin"), dst="bin")
                    copy(self, "*.dll", src=os.path.join(dep.package_folder, "bin"), dst="bin")
                    # copy(self, "*.lib", src=os.path.join(dep.package_folder, "lib"), dst="lib")
                    # Copy DLLs and other things from older pre-builts that use binr/bind
                    copy(self, "*", dst="bin", src=os.path.join(dep.package_folder, "binr"))
                if self.settings.os == "Macos":
                    copy(self, "*", src=os.path.join(dep.package_folder, "include"), dst="include")
                    copy(self, "*", src=os.path.join(dep.package_folder, "lib"), dst="lib")
