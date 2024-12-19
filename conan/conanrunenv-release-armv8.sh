script_folder="/Users/joshyoo/Desktop/Topaz-Code/mpv/conan"
echo "echo Restoring environment" > "$script_folder/deactivate_conanrunenv-release-armv8.sh"
for v in LD_LIBRARY_PATH DYLD_LIBRARY_PATH
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


export LD_LIBRARY_PATH="/Users/joshyoo/.conan2/p/libjp36d48aceb64a1/p/lib:$LD_LIBRARY_PATH"
export DYLD_LIBRARY_PATH="/Users/joshyoo/.conan2/p/libjp36d48aceb64a1/p/lib:$DYLD_LIBRARY_PATH"