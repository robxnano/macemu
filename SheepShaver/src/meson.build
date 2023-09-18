configure_file(output: 'config.h', configuration: config_h)

inc_dirs = ['include', 'kpx_cpu', 'kpx_cpu/include', 'kpx_cpu/src', 'Unix', 'CrossPlatform']

src = [
  'CrossPlatform/vm_alloc.cpp',
  'CrossPlatform/sigsegv.cpp',
  'CrossPlatform/video_blit.cpp',
  'main.cpp',
  'prefs.cpp',
  'prefs_items.cpp',
  'rom_patches.cpp',
  'rsrc_patches.cpp',
  'emul_op.cpp',
  'name_registry.cpp',
  'macos_util.cpp',
  'timer.cpp',
  'xpram.cpp',
  'adb.cpp',
  'sony.cpp',
  'disk.cpp',
  'cdrom.cpp',
  'scsi.cpp',
  'gfxaccel.cpp',
  'video.cpp',
  'audio.cpp',
  'ether.cpp',
  'thunks.cpp',
  'serial.cpp',
  'extfs.cpp',
  'Unix/tinyxml2.cpp',
  'user_strings.cpp',
]

if host_system != 'windows'
  src += [
    'Unix/main_unix.cpp',
    'Unix/prefs_unix.cpp',
    'Unix/sys_unix.cpp',
    'Unix/timer_unix.cpp',
    'Unix/xpram_unix.cpp',
    'Unix/disk_sparsebundle.cpp',
    'Unix/about_window_unix.cpp',
    'Unix/user_strings_unix.cpp',
    'Unix/rpc_unix.cpp',
    'Unix/sshpty.c',
    'Unix/strlcpy.c',
    'Unix/serial_unix.cpp',
    'Unix/ether_unix.cpp',
    'Unix/Linux/scsi_linux.cpp',
    'Unix/prefs_unix.cpp',
    'Unix/extfs_unix.cpp',
  ]
endif

if not get_option('gtk').disabled()
  gnome = import('gnome')
  src += [
    'dummy/prefs_dummy.cpp',
    'prefs.cpp',
    'Unix/prefs_unix.cpp',
    'Unix/prefs_editor_gtk3.cpp',
    'prefs_items.cpp',
    'user_strings.cpp',
    'Unix/user_strings_unix.cpp',
    'Unix/xpram_unix.cpp',
    'Unix/sys_unix.cpp',
    'Unix/color_scheme.cpp',
  ]
  ui_xml = files(
    'Unix/ui/help-overlay.ui',
    'Unix/ui/menu.ui',
    'Unix/ui/prefs-editor.ui',
  )
  src += gnome.compile_resources('g_resource', 'Unix/ui/sheepshaver.gresource.xml',
    c_name: 'sheepshaver',
    source_dir: 'Unix',
    dependencies: ui_xml)
else
  src += 'dummy/prefs_dummy.cpp'
endif

if not get_option('slirp').disabled()
  src += [
    'slirp/bootp.c',
    'slirp/bootp.c',
    'slirp/ip_output.c',
    'slirp/tcp_input.c',
    'slirp/cksum.c',
    'slirp/mbuf.c',
    'slirp/tcp_output.c',
    'slirp/debug.c',
    'slirp/misc.c',
    'slirp/tcp_subr.c',
    'slirp/if.c',
    'slirp/sbuf.c',
    'slirp/tcp_timer.c',
    'slirp/ip_icmp.c',
    'slirp/slirp.c',
    'slirp/tftp.c',
    'slirp/ip_input.c',
    'slirp/socket.c',
    'slirp/udp.c',
  ]
endif

if not get_option('sdl').disabled()
  inc_dirs += 'slirp'
  src += [
    'SDL/video_sdl.cpp',
    'SDL/video_sdl2.cpp',
    'SDL/audio_sdl.cpp',
    'dummy/clip_dummy.cpp',
  ]
  keycodes = 'SDL/keycodes'
elif host_system == 'darwin'
  keycodes = 'MacOSX/keycodes'
else
  keycodes = 'Unix/keycodes'
endif


if not get_option('bincue').disabled()
  src += 'bincue.cpp'
endif

if not host_machine.cpu_family().startswith('ppc')
  src += [
    'kpx_cpu/src/mathlib/ieeefp.cpp',
    'kpx_cpu/src/mathlib/mathlib.cpp',
    'kpx_cpu/src/cpu/ppc/ppc-cpu.cpp',
    'kpx_cpu/src/cpu/ppc/ppc-decode.cpp',
    'kpx_cpu/src/cpu/ppc/ppc-execute.cpp',
    'kpx_cpu/src/cpu/ppc/ppc-translate.cpp',
    'kpx_cpu/src/utils/utils-cpuinfo.cpp',
  ]
  if use_dyngen == 'precompiled'
    if host_machine.cpu_family().startswith('x86')
      add_project_arguments('-fomit-frame-pointer', language: ['c', 'cpp'])
    endif
    src += [
      'kpx_cpu/src/cpu/jit/jit-cache.cpp',
      'kpx_cpu/src/cpu/jit/basic-dyngen.cpp',
      'kpx_cpu/src/cpu/ppc/ppc-dyngen.cpp',
      'kpx_cpu/src/cpu/ppc/ppc-jit.cpp',
      'kpx_cpu/sheepshaver_glue.cpp',
      'kpx_cpu/ppc-dis.c',
    ]
  endif
endif

dyngen_src = ['kpx_cpu/src/cpu/jit/dyngen.c', 'kpx_cpu/src/cpu/jit/cxxdemangle.cpp']

executable('SheepShaver',
  sources: src,
  dependencies: sheepshaver_deps,
  include_directories: inc_dirs,
)

install_data(keycodes,
  install_dir: get_option('datadir')/'SheepShaver')

install_man('Unix/SheepShaver.1')