project(
    'river-status-bar',
    ['c', 'cpp'],
    version: '0.1',
    default_options: ['warning_level=3', 'cpp_std=c++23'],
)

inc_dir = include_directories('include')
cpp_src_files = files(
    'src/main.cpp',
    'src/wayland-state.cpp',
    'src/utils.cpp'
)

c_src_files = files(
    'src/wayland/xdg-shell.c',
    'src/wayland/zwlr-layer-shell.c',
    'src/wayland/zriver-protocol.c'
)

wayland_dep = dependency('wayland-client')
spdlog_dep = dependency('spdlog')
cairo_dep = dependency('cairo')
pango_dep = dependency('pango')

spdlog_defines = ['-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE']

if get_option('buildtype') == 'release'
    # remove debug statements from release build
    spdlog_defines = ['-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN']

    # add optimization flags to release build cpp
    add_project_arguments(
        '-flto=auto',
        '-fwhole-program',
        '-funroll-loops',
        '-ffast-math',
        '-Ofast',
        '-march=native',
        '-fmerge-all-constants',
        language: 'cpp',
    )

    # add optimization flags to release build c
    add_project_arguments(
        '-flto=auto', # Link Time Optimization
        '-fwhole-program', # Whole program optimization
        '-funroll-loops', # Unroll loops
        '-ffast-math', # Enable fast math (potential precision loss)
        '-march=native', # Target the local architecture
        '-fomit-frame-pointer', # Omit frame pointer for better performance
        '-fstrict-aliasing', # Assume strict aliasing (better optimizations)
        '-ftree-vectorize', # Enable vectorization
        '-falign-functions=16', # Align functions to 16-byte boundaries
        '-falign-loops=16', # Align loops to 16-byte boundaries
        '-fno-common', # Treat global variables as separate
        '-fmerge-all-constants',
        language: 'c',
    )
endif

# executable version of model
executable(
    'river-status-bar',
    cpp_src_files
    + c_src_files,
    include_directories: inc_dir,
    dependencies: [spdlog_dep, wayland_dep, cairo_dep, pango_dep],
    cpp_args: spdlog_defines,
    install: false,
)