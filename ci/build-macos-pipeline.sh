#!/usr/bin/env bash

set -e

VERSION=$1
PUBLISH=$2
TOPAZ_CONAN=$3

MPV_INSTALL_PREFIX="$(pwd)/build/out/mpv"
MPV_VARIANT="1"

if [[ -d "./build/${MPV_VARIANT}" ]] ; then
    rm -rf "./build/${MPV_VARIANT}"
fi



# set conan path
mv -r "../conan_arm/" "../conan/"
# Do armv8 first
meson setup build \
    --wrap-mode=forcefallback \
    -Dprefix="${MPV_INSTALL_PREFIX}" \
    -Ddefault_library=shared \
    -D{c_args,objc_args}="-Wno-error=deprecated -Wno-error=deprecated-declarations" \
    -D{libmpv,mac}=true \
    -D{gl,iconv,lcms2,plain-gl,zlib}=enabled \
    -Djpeg=disabled \
    -Dtests=false \
    -Dlibass:mac=true \
    -Dlcms2:fastfloat=true \
    -Dlcms2:jpeg=disabled \
    -Dlcms2:tiff=disabled \
    -Dlibusb:tests=false \
    -Dlibusb:examples=false \
    -Dlibplacebo:mac=true \
    -Dlibplacebo:demos=false \
    -Dlibplacebo:lcms=enabled \
    -Dlibplacebo:shaderc=enabled \
    -Dlibplacebo:d3d11=disabled \
    -Dlibplacebo:dovi=disabled \
    -Dlibplacebo:libdovi=disabled \
    -Dlibplacebo:xxhash=disabled \
    -Dlibplacebo:vulkan=enabled \
    -Dxxhash:inline-all=true \
    -Dxxhash:cli=false \
    -Ddrm=disabled \
    -Dlibarchive=disabled \
    -Drubberband=disabled \
    -Dwayland=disabled \
    -Dx11=disabled \
    -Dwin32-smtc=disabled \
    -Dvulkan=enabled \
    -Dlua=disabled \
    -D{cocoa,coreaudio,gl-cocoa,videotoolbox-gl,videotoolbox-pl}=enabled \
    -D{swift-build,macos-cocoa-cb,macos-media-player,macos-touchbar}=enabled 

meson compile -C build -j4
meson install -C build
# ./build/mpv -v --no-config

if ["$PUBLISH" -eq 1 ]; then

    cd ../

    mkdir -p ${TOPAZ_CONAN}/prebuilt/topaz-ffmpeg/${VERSION}/profile_mac_armv8/build_type\=Release/
	cp -Rp build/out/mpv/* ${TOPAZ_CONAN}/prebuilt/topaz-ffmpeg/${VERSION}/profile_mac_armv8/build_type\=Release/

    cp build-scripts/deploy_conanfile.py ${TOPAZ_CONAN}/prebuilt/topaz-ffmpeg/${VERSION}/conanfile.py
fi

cd $(Build.SourcesDirectory)

rm -rf ./build
rm -rf ./conan

# Armv8 x x86_64 cross compile
mv -r "./conan_x64/" "./conan/"

meson setup build \
    --cross-file ci/x86_64-cross-file.txt \
    --wrap-mode=forcefallback \
    -Dprefix="${MPV_INSTALL_PREFIX}" \
    -Ddefault_library=shared \
    -D{c_args,objc_args}="-Wno-error=deprecated -Wno-error=deprecated-declarations" \
    -D{libmpv,mac}=true \
    -D{gl,iconv,lcms2,plain-gl,zlib}=enabled \
    -Djpeg=disabled \
    -Dtests=false \
    -Dlibass:mac=true \
    -Dlcms2:fastfloat=true \
    -Dlcms2:jpeg=disabled \
    -Dlcms2:tiff=disabled \
    -Dlibusb:tests=false \
    -Dlibusb:examples=false \
    -Dlibplacebo:mac=true \
    -Dlibplacebo:demos=false \
    -Dlibplacebo:lcms=enabled \
    -Dlibplacebo:shaderc=enabled \
    -Dlibplacebo:d3d11=disabled \
    -Dlibplacebo:dovi=disabled \
    -Dlibplacebo:libdovi=disabled \
    -Dlibplacebo:xxhash=disabled \
    -Dlibplacebo:vulkan=enabled \
    -Dxxhash:inline-all=true \
    -Dxxhash:cli=false \
    -Ddrm=disabled \
    -Dlibarchive=disabled \
    -Drubberband=disabled \
    -Dwayland=disabled \
    -Dx11=disabled \
    -Dwin32-smtc=disabled \
    -Dvulkan=enabled \
    -Dlua=disabled \
    -D{cocoa,coreaudio,gl-cocoa,videotoolbox-gl,videotoolbox-pl}=enabled \
    -D{swift-build,macos-cocoa-cb,macos-media-player,macos-touchbar}=enabled 

meson compile -C build -j4
meson install -C build
# ./build/mpv -v --no-config

if ["$PUBLISH" -eq 1 ]; then

    cd ../..

    mkdir -p ${TOPAZ_CONAN}/prebuilt/topaz-ffmpeg/${VERSION}/profile_mac14.0/build_type\=Release/
	cp -Rp build/out/mpv/* ${TOPAZ_CONAN}/prebuilt/topaz-ffmpeg/${VERSION}/profile_mac14.0/build_type\=Release/

    cp build-scripts/deploy_conanfile.py ${TOPAZ_CONAN}/prebuilt/topaz-ffmpeg/${VERSION}/conanfile.py


    cd ${TOPAZ_CONAN}
    bash ./run_publish_prebuilt.sh --package-name topaz-ffmpeg --package-version ${VERSION} -r topaz-conan

fi

