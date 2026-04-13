from conan import ConanFile
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
from conan.tools.files import copy
from conan.tools.gnu import PkgConfigDeps
import os


required_conan_version = ">=2.0"


class MPVConan(ConanFile):
    name = "mpv"
    description = ""
    # topics = "gio", "gmodule", "gnome", "gobject", "gtk"
    # url = "https://github.com/conan-io/conan-center-index"
    # homepage = "https://gitlab.gnome.org/GNOME/glib"
    # license = "LGPL-2.1-or-later"
    settings = "os", "arch", "compiler", "build_type"
    options = {
    }
    default_options = {
    }

    def configure(self):
        self.options["libiconv"].shared = True
        # self.options['glib'].shared = True
        self.options["harfbuzz"].shared = False
        self.options["harfbuzz"].with_glib=False
        self.options["fribidi"].shared = True
        # self.options["lcms"].shared = True

        self.settings.rm_safe("compiler.cppstd")
        self.settings.rm_safe("compiler.libcxx")


    def requirements(self):
        self.requires("topaz-ffmpeg/8.0.1.2")
        self.requires("topaz_rlm/0.1.2", override=True)
        self.requires("videoai/2.0.19", override=True)
        self.requires("aiengine/3.8.14", override=True)
        self.requires("zlib/1.2.13")
        self.requires("harfbuzz/8.3.0-topaz")
        self.requires("fribidi/1.0.13-topaz") # LGPL
        # self.requires("lcms/2.17")
        self.requires("libiconv/1.17") # LGPL
    
    def generate(self):
        # Generate the pkg-config metadata and env wrappers Meson consumes.
        PkgConfigDeps(self).generate()
        VirtualBuildEnv(self).generate()
        VirtualRunEnv(self).generate()

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
