project('cgdrv_prebuilt', 'c')

# decompress the drivers
run_command(find_program('tar', required: true),
  '-xjf', 'nv_tegra/nvidia_drivers.tbz2',
  '--strip-components=2', 'usr/lib',
  check: true
)

lib_cgdrv = meson.get_compiler('c').find_library(
  'cgdrv',
  dirs : [meson.current_source_dir()]
)

cgdrv_env = environment()
cgdrv_env.append('LD_LIBRARY_PATH', meson.current_source_dir())
meson.add_devenv(cgdrv_env)

cgdrv_dep = declare_dependency(
  dependencies : lib_cgdrv
)

meson.override_dependency('cgdrv', cgdrv_dep)
