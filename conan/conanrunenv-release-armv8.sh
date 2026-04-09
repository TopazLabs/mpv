script_folder="/Users/joshyoo/Desktop/Topaz-Code/mpv/conan"
echo "echo Restoring environment" > "$script_folder/deactivate_conanrunenv-release-armv8.sh"
for v in PATH LD_LIBRARY_PATH DYLD_LIBRARY_PATH OPENSSL_MODULES
do
    is_defined="true"
    value=$(printenv $v) || is_defined="" || true
    if [ -n "$value" ] || [ -n "$is_defined" ]
    then
        echo export "$v='$value'" >> "$script_folder/deactivate_conanrunenv-release-armv8.sh"
    else
        echo unset $v >> "$script_folder/deactivate_conanrunenv-release-armv8.sh"
    fi
done


export PATH="/Users/joshyoo/.conan2/p/opencbd6194c73b051/p/bin:/Users/joshyoo/.conan2/p/libzic004089ffeeac/p/bin:/Users/joshyoo/.conan2/p/opens2ddc8141113bb/p/bin:/Users/joshyoo/.conan2/p/freet1eb7cea5a1f34/p/bin:/Users/joshyoo/.conan2/p/glibdb7600dd2ad33/p/bin:/Users/joshyoo/.conan2/p/libic8e951695a6500/p/bin:$PATH"
export LD_LIBRARY_PATH="/Users/joshyoo/.conan2/p/opencbd6194c73b051/p/lib:/Users/joshyoo/.conan2/p/libti1eecbce31a596/p/lib:/Users/joshyoo/.conan2/p/libzic004089ffeeac/p/lib:/Users/joshyoo/.conan2/p/opens2ddc8141113bb/p/lib:/Users/joshyoo/.conan2/p/zimg71c0081c4544c/p/lib:/Users/joshyoo/.conan2/p/libvpbdf6f6c8b52aa/p/lib:/Users/joshyoo/.conan2/p/lcms8ac8b32768bc2/p/lib:/Users/joshyoo/.conan2/p/harfb6aaf847b76e0f/p/lib:/Users/joshyoo/.conan2/p/freet1eb7cea5a1f34/p/lib:/Users/joshyoo/.conan2/p/glibdb7600dd2ad33/p/lib:/Users/joshyoo/.conan2/p/libic8e951695a6500/p/lib:$LD_LIBRARY_PATH"
export DYLD_LIBRARY_PATH="/Users/joshyoo/.conan2/p/opencbd6194c73b051/p/lib:/Users/joshyoo/.conan2/p/libti1eecbce31a596/p/lib:/Users/joshyoo/.conan2/p/libzic004089ffeeac/p/lib:/Users/joshyoo/.conan2/p/opens2ddc8141113bb/p/lib:/Users/joshyoo/.conan2/p/zimg71c0081c4544c/p/lib:/Users/joshyoo/.conan2/p/libvpbdf6f6c8b52aa/p/lib:/Users/joshyoo/.conan2/p/lcms8ac8b32768bc2/p/lib:/Users/joshyoo/.conan2/p/harfb6aaf847b76e0f/p/lib:/Users/joshyoo/.conan2/p/freet1eb7cea5a1f34/p/lib:/Users/joshyoo/.conan2/p/glibdb7600dd2ad33/p/lib:/Users/joshyoo/.conan2/p/libic8e951695a6500/p/lib:$DYLD_LIBRARY_PATH"
export OPENSSL_MODULES="/Users/joshyoo/.conan2/p/opens2ddc8141113bb/p/lib/ossl-modules"