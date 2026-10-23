from conan import ConanFile
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv
from conan.tools.files import copy
from conan.tools.gnu import PkgConfigDeps
import os


required_conan_version = ">=2.0"
FILTERED_HEADER_DIRS = ("aiengine", "videoai")
# macOS: packages whose files are staged into conan/lib3rdparty/<name>
MAC_STAGED = ("topaz-ffmpeg", "libiconv", "fribidi", "harfbuzz", "freetype", "zlib", "lcms")


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
        # Every LGPL library must be a shared library: FFmpeg, libplacebo (meson
        # subproject), fribidi, libiconv. freetype/harfbuzz are shared too so libass
        # does not embed copies. zlib and lcms stay static, the same binaries
        # videoenhanceai uses. Same options on Windows and macOS.
        self.options["libiconv"].shared = True
        self.options["fribidi"].shared = True
        self.options["harfbuzz"].shared = True
        self.options["harfbuzz"].with_glib = False
        self.options["freetype"].shared = True
        self.options["freetype"].with_png = False
        self.options["freetype"].with_bzip2 = False
        self.options["freetype"].with_brotli = False
        self.options["freetype"].with_zlib = False

        if self.settings.os == "Windows":
            self.options["shaderc"].shared = True
            self.options["spirv-cross"].shared = True
            # same as videoenhanceai/deploy/conanfile.py
            self.options["topaz-ffmpeg"].tensorrt_rtx = True

        self.settings.rm_safe("compiler.cppstd")
        self.settings.rm_safe("compiler.libcxx")

    def requirements(self):
        self.requires("topaz-ffmpeg/[~8.1.0.0]")
        self.requires("zlib/1.2.13")
        self.requires("harfbuzz/8.3.0-topaz")
        self.requires("fribidi/1.0.13-topaz")  # LGPL
        self.requires("libiconv/1.17")  # LGPL
        self.requires("lcms/2.17")  # libplacebo ICC support
        if self.settings.os == "Windows":
            # Shader compilers for libplacebo's d3d11 backend
            self.requires("shaderc/2024.1")
            self.requires("spirv-cross/1.3.268.0")

    def build_requirements(self):
        if self.settings.os == "Windows":
            self.tool_requires("meson/1.4.0")
            self.tool_requires("ninja/1.11.1")
            self.tool_requires("pkgconf/2.1.0")
            self.tool_requires("nasm/2.16.01")  # libass asm

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
                    # meson.build adds conan/include to the public include dirs
                    copy(
                        self,
                        "*",
                        src=os.path.join(dep.package_folder, "include"),
                        dst="include",
                        excludes=[f"{name}/*" for name in FILTERED_HEADER_DIRS],
                    )
                if self.settings.os == "Macos":
                    # topaz-ffmpeg brings its whole runtime (videoai, opencv, ...) in as
                    # transitive deps; mpv only builds against FFmpeg and the libraries
                    # below, everything else is found through the generated .pc files.
                    name = str(dep.ref).split("/")[0]
                    if name in MAC_STAGED:
                        excludes = [f"{n}/*" for n in FILTERED_HEADER_DIRS]
                        include = os.path.join(dep.package_folder, "include")
                        dst = os.path.join("lib3rdparty", name)
                        copy(self, "*", src=os.path.join(dep.package_folder, "lib"), dst=os.path.join(dst, "lib"))
                        copy(self, "*", src=include, dst=os.path.join(dst, "include"), excludes=excludes)
                        copy(self, "*", src=os.path.join(dep.package_folder, "licenses"), dst=os.path.join(dst, "licenses"))
                        # meson.build adds conan/include to the public include dirs
                        copy(self, "*", src=include, dst="include", excludes=excludes)
