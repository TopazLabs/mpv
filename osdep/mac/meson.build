# custom swift targets
bridge = join_paths(source_root, 'osdep/mac/app_bridge_objc.h')
module = join_paths(build_root, 'osdep/mac/swift.swiftmodule')
target = join_paths(build_root, 'osdep/mac/swift.o')

# add_project_arguments('-D_DARWIN_C_SOURCE', language: 'c')

# if host_machine.system() == 'darwin'
#     if meson.is_cross_build()
#         add_global_link_arguments('-mmacosx-version-min=11.0', language: ['c', 'cpp', 'objc', 'objcpp'])
#         add_global_arguments('-mmacosx-version-min=11.0', language: ['c', 'cpp', 'objc', 'objcpp'])
#         add_global_arguments(['-arch','x86_64'], language: ['c', 'cpp', 'objc', 'objcpp', 'swift'])
#         add_global_link_arguments(['-arch','x86_64'], language: ['c', 'cpp', 'objc', 'objcpp', 'swift'])
#     endif
# endif

swift_flags = ['-c', '-emit-library', '-static', '-sdk', macos_sdk_path,
               '-emit-objc-header', '-parse-as-library',
               '-Xcc', '-I' + meson.global_source_root() + '/build/subprojects/libplacebo/src/include',
  '-Xcc', '-I' + meson.global_source_root() + '/conan/include']
  

if swift_ver.version_compare('>=6.0')
    swift_flags += ['-swift-version', '5']
endif

if get_option('debug')
    swift_flags += '-g'
endif

if get_option('optimization') != '0'
    swift_flags += '-O'
endif

foreach feature, allowed: features
    if allowed
        swift_flags += ['-D', 'HAVE_@0@'.format(feature.underscorify().to_upper())]
    endif
endforeach

if not meson.is_cross_build()
    swift_flags += ['-target', 'arm64-apple-macosx11.0']
elif meson.is_cross_build()
    swift_flags += ['-target', 'x86_64-apple-macosx11.0']
endif

extra_flags = get_option('swift-flags').split()
swift_flags += extra_flags
message('***** LIBPLACEBO: ',libplacebo.get_variable('includedir',default_value: source_root / 'subprojects' / 'libplacebo' / 'src' / 'include'))
# libavutil_inc = include_directories(source_root / 'conan' / 'include' / 'libavutil')
# libavutil_inc = include_directories('../../conan/include/libavutil')
# libavutil_inc = source_root / 'conan' / 'include'
libavutil_inc = meson.global_source_root() + '/conan/include'
message('***** LIBAVUTIL: ', libavutil_inc)

swift_compile = [swift_prog, swift_flags, '-module-name', 'swift',
                 '-emit-module-path', '@OUTPUT0@', '-import-objc-header', bridge,
                 '-emit-objc-header-path', '@OUTPUT1@', '-o', '@OUTPUT2@',
                 '@INPUT@', '-I.', '-I' + source_root, '-I' + source_root / 'include',
                #  '-I' + libplacebo.get_variable('includedir',
                #             default_value: source_root / 'subprojects' / 'libplacebo' / 'src' / 'include'),
                #  '-I' + libavutil.get_variable('includedir')]
                #     '-I' + libplacebo.get_variable('includedir',
                #             default_value: source_root / 'subprojects' / 'libplacebo' / 'src' / 'include'),
                #  '-I' + libavutil_inc]
                    '-I' + libplacebo.get_variable('includedir',
                            default_value: meson.global_source_root() + '/subprojects/libplacebo/src/include'),
                 '-I' + libavutil_inc]

swift_targets = custom_target('swift_targets',
    input: swift_sources,
    output: ['swift.swiftmodule', 'swift.h', 'swift.o'],
    command: swift_compile,
)
sources += swift_targets

swift_lib_dir_py = find_program(join_paths(tools_directory, 'macos-swift-lib-directory.py'))
swift_lib_dir = run_command(swift_lib_dir_py, swift_prog.full_path(), check: true).stdout()
message('Detected Swift library directory: ' + swift_lib_dir)

# linker flags
swift_link_flags = ['-L' + swift_lib_dir, '-Xlinker', '-rpath',
                    '-Xlinker', swift_lib_dir, '-rdynamic', '-Xlinker',
                    '-add_ast_path', '-Xlinker', module]
if swift_ver.version_compare('>=5.0')
    swift_link_flags += ['-Xlinker', '-rpath', '-Xlinker',
                         '/usr/lib/swift', '-L/usr/lib/swift']
endif

# if not meson.is_cross_build() 
#     swift_link_flags += ['-mmacosx-version-min=11']
# elif meson.is_cross_build()
#     swift_link_flags += ['-mmacosx-version-min=10.15']
# endif

add_project_link_arguments(swift_link_flags, language: ['c', 'objc'])
