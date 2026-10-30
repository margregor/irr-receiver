project('irremote-volume', 'cpp',
  default_options : ['cpp_std=c++20', 'buildtype=release', 'warning_level=3'])

if host_machine.system() != 'windows'
  error('irremote-volume is a Windows-only application')
endif

windows = import('windows')

win_resources = windows.compile_resources('resources.rc')

executable(
  'irremote-volume',
  'irremote_volume.cpp',
  win_resources,
  win_subsystem : 'windows',
  install : false,
)
