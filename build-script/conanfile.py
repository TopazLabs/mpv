from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import fix_apple_shared_install_name, is_apple_os
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import (
    apply_conandata_patches,
    copy,
    export_conandata_patches,
    get,
    replace_in_file,
    rm,
    rmdir,
    mkdir
)
from conan.tools.gnu import PkgConfigDeps
from conan.tools.layout import basic_layout
from conan.tools.meson import Meson, MesonToolchain
from conan.tools.microsoft import is_msvc
from conan.tools.scm import Version
import os
import shutil


required_conan_version = ">=1.53.0"


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
        self.options["freetype"].shared = True
        self.options["freetype"].with_brotli = True
        self.options["freetype"].with_png = True
        
        self.options["harfbuzz"].shared = True
        self.options["harfbuzz"].with_glib_iconv_shared = True

        self.options["libiconv"].shared = True
        self.options["libjpeg"].shared = True
        self.options["lcms"].shared = True

        
        self.settings.rm_safe("compiler.cppstd")
        self.settings.rm_safe("compiler.libcxx")


    def requirements(self):
        # self.requires("topaz-ffmpeg/7.0.2.4")
        self.requires("zlib/1.2.13")
        self.requires("lcms/2.14")
        self.requires("libiconv/1.17") # new shared
        self.requires("freetype/2.13.2")
        self.requires("fribidi/1.0.13@josh/mpv")
        self.requires("libjpeg/9e")
        self.requires("harfbuzz/8.3.0@josh/mpv") # new shared (with new options... also in glib)
    
    def generate(self):
        # pc = PkgConfigDeps(self)
        # pc.install_folder = os.path.join(self.build_folder, "pkgconfig")
        # mkdir(self, pc.install_folder)
        # pc.generate()
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
