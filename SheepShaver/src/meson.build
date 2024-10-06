configure_file(output: 'config.h', configuration: config_h)

inc_dirs = ['include', 'kpx_cpu', 'kpx_cpu/include', 'kpx_cpu/src', 'CrossPlatform']
if host_system == 'windows'
  inc_dirs += 'Windows'
else
  inc_dirs += 'Unix'
endif

ui_src = []
src = [
  'adb.cpp',
  'audio.cpp',
  'cdrom.cpp',
  'disk.cpp',
  'emul_op.cpp',
  'ether.cpp',
  'extfs.cpp',
  'gfxaccel.cpp',
  'macos_util.cpp',
  'main.cpp',
  'name_registry.cpp',
  'prefs.cpp',
  'prefs_items.cpp',
  'rom_patches.cpp',
  'rsrc_patches.cpp',
  'scsi.cpp',
  'serial.cpp',
  'sony.cpp',
  'thunks.cpp',
  'timer.cpp',
  'user_strings.cpp',
  'video.cpp',
  'xpram.cpp',
  'CrossPlatform/sigsegv.cpp',
  'CrossPlatform/video_blit.cpp',
  'CrossPlatform/vm_alloc.cpp',
  'dummy/prefs_dummy.cpp',
]

if host_system == 'windows'
  src += [
    'dummy/prefs_editor_dummy.cpp',
    'dummy/scsi_dummy.cpp',
    'Windows/clip_windows.cpp',
    'Windows/ether_windows.cpp',
    'Windows/extfs_windows.cpp',
    'Windows/main_windows.cpp',
    'Windows/posix_emu.cpp',
    'Windows/prefs_windows.cpp',
    'Windows/serial_windows.cpp',
    'Windows/sys_windows.cpp',
    'Windows/timer_windows.cpp',
    'Windows/user_strings_windows.cpp',
    'Windows/util_windows.cpp',
    'Windows/xpram_windows.cpp',
    'Windows/b2ether/packet32.cpp',
    'Windows/cdenable/cache.cpp',
    'Windows/cdenable/eject_nt.cpp',
    'Windows/cdenable/ntcd.cpp',
    'Windows/router/arp.cpp',
    'Windows/router/dump.cpp',
    'Windows/router/dynsockets.cpp',
    'Windows/router/ftp.cpp',
    'Windows/router/icmp.cpp',
    'Windows/router/mib/interfaces.cpp',
    'Windows/router/iphelp.cpp',
    'Windows/router/ipsocket.cpp',
    'Windows/router/mib/mibaccess.cpp',
    'Windows/router/router.cpp',
    'Windows/router/tcp.cpp',
    'Windows/router/udp.cpp',
  ]
else
  src += [
    'Unix/about_window_unix.cpp',
    'Unix/disk_sparsebundle.cpp',
    'Unix/ether_unix.cpp',
    'Unix/extfs_unix.cpp',
    'Unix/main_unix.cpp',
    'Unix/prefs_unix.cpp',
    'Unix/rpc_unix.cpp',
    'Unix/serial_unix.cpp',
    'Unix/sshpty.c',
    'Unix/strlcpy.c',
    'Unix/sys_unix.cpp',
    'Unix/timer_unix.cpp',
    'Unix/tinyxml2.cpp',
    'Unix/user_strings_unix.cpp',
    'Unix/xpram_unix.cpp',
  ]
  if host_system == 'linux'
    src += 'Unix/Linux/scsi_linux.cpp'
  else
    src += 'dummy/scsi_dummy.cpp'
  endif
endif

if use_gtk
  if gtk.version().version_compare('>= 3.0')
    gnome = import('gnome')
    ui_xml = files(
      'Unix/ui/help-overlay.ui',
      'Unix/ui/menu.ui',
      'Unix/ui/prefs-editor.ui',
    )
    ui = gnome.compile_resources('g_resource', 'Unix/ui/sheepshaver.gresource.xml',
      c_name: 'sheepshaver',
      source_dir: 'Unix',
      dependencies: ui_xml)
    ui_src += [ui, 'Unix/prefs_editor_gtk3.cpp', 'Unix/color_scheme.cpp']
  elif host_system == 'windows'
    ui_src += [
      'prefs.cpp',
      'Windows/prefs_editor_gtk.cpp',
      'Windows/prefs_windows.cpp',
      'Windows/xpram_windows.cpp',
      'prefs_items.cpp',
      'user_strings.cpp',
      'Windows/user_strings_windows.cpp',
      'Windows/util_windows.cpp',
      'dummy/prefs_dummy.cpp',
      'Windows/b2ether/packet32.cpp',
    ]
  else
    ui_src += 'Unix/prefs_editor_gtk.cpp'
  endif
endif

if get_option('slirp') or host_system == 'windows'
  inc_dirs += 'slirp'
  add_project_arguments('-fno-strict-aliasing', language: 'c')
  src += [
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

if get_option('sdl') != 'disabled'
  src += [
    'SDL/video_sdl.cpp',
    'SDL/video_sdl2.cpp',
    'SDL/audio_sdl.cpp',
  ]
  if host_system != 'windows'
    src += 'dummy/clip_dummy.cpp'
  endif
  keycodes = 'SDL/keycodes'
elif host_system == 'darwin'
  src += [
    #TODO: Finish this
  ]
  keycodes = 'MacOSX/keycodes'
else # X11 on Unix
  src += [
    'Unix/video_x.cpp',
    'Unix/clip_unix.cpp',
    'dummy/audio_dummy.cpp',
  ]
  keycodes = 'Unix/keycodes'
endif

if not get_option('bincue').disabled()
  src += 'bincue.cpp'
endif

if not host_machine.cpu_family().startswith('ppc')
  if use_dyngen == 'precompiled'
    if host_machine.cpu_family().startswith('x86')
      add_project_arguments('-fomit-frame-pointer', language: ['c', 'cpp'])
    endif
    if host_system == 'windows'
      inc_dirs += 'Windows/cygwin_precompiled_dyngen'
    else
      inc_dirs += 'Unix/dyngen_precompiled'
    endif
    src += [
      'kpx_cpu/src/cpu/jit/jit-cache.cpp',
      'kpx_cpu/src/cpu/jit/basic-dyngen.cpp',
      'kpx_cpu/src/cpu/ppc/ppc-dyngen.cpp',
      'kpx_cpu/src/cpu/ppc/ppc-jit.cpp',
    ]
  elif use_dyngen == 'enabled'
    dyngen_args = [

    ]
    dyngen_src = ['kpx_cpu/src/cpu/jit/dyngen.c', 'kpx_cpu/src/cpu/jit/cxxdemangle.cpp']
    dyngen = executable('dyngen', dyngen_src,
      include_directories: inc_dirs,
      c_args: dyngen_args,
      cpp_args: dyngen_args,
    )

    dyngen_cpp_args = [
      '-fomit-frame-pointer',
      '-fno-align-functions',
      '-fno-stack-protector',
      '-finline-functions',
      '-finline-limit=10000',
      '-fno-exceptions',
      '-g0',
      '-fno-reorder-blocks',
      '-fno-optimize-sibling-calls',
      '-fno-reorder-blocks-and-partition',
    ]

    basic_dyngen_ops_a = static_library('basic-dyngen-ops',
      'kpx_cpu/src/cpu/jit/basic-dyngen-ops.cpp',
      cpp_args: dyngen_cpp_args,
      include_directories: inc_dirs,
      pic: false)

    basic_dyngen_ops_o = basic_dyngen_ops_a.extract_objects('kpx_cpu/src/cpu/jit/basic-dyngen-ops.cpp')

    basic_dyngen_ops = custom_target('basic-dyngen-ops',
      command: [dyngen, '-o', '@OUTPUT@', '@INPUT@'],
      input: basic_dyngen_ops_o,
      output: 'basic-dyngen-ops.hpp',
    )

    ppc_dyngen_ops_a = static_library('ppc-dyngen-ops', basic_dyngen_ops,
      'kpx_cpu/src/cpu/ppc/ppc-dyngen-ops.cpp',
      cpp_args: dyngen_cpp_args,
      include_directories: inc_dirs,
      pic: false)

    ppc_dyngen_ops_o = ppc_dyngen_ops_a.extract_objects('kpx_cpu/src/cpu/ppc/ppc-dyngen-ops.cpp')

    ppc_dyngen_ops = custom_target('ppc-dyngen-ops',
      command: [dyngen, '-o', '@OUTPUT@', '@INPUT@'],
      input: ppc_dyngen_ops_o,
      output: 'ppc-dyngen-ops.hpp',
    )

    src += [
      basic_dyngen_ops,
      ppc_dyngen_ops,
      'kpx_cpu/src/cpu/jit/jit-cache.cpp',
      'kpx_cpu/src/cpu/jit/basic-dyngen.cpp',
      'kpx_cpu/src/cpu/ppc/ppc-dyngen.cpp',
      'kpx_cpu/src/cpu/ppc/ppc-jit.cpp',
    ]
  endif

  perl = find_program('perl')
  genexecpl = files('kpx_cpu/src/cpu/ppc/genexec.pl')
  inc_dirs += 'kpx_cpu/src/cpu/ppc'
  ppc_preprocessed = cxx.preprocess('kpx_cpu/src/cpu/ppc/ppc-decode.cpp',
    include_directories: include_directories(inc_dirs))

  ppc_impl = custom_target('ppc-execute-impl',
    command: [perl, genexecpl],
    input: ppc_preprocessed,
    output: 'ppc-execute-impl.hpp',
    depend_files: genexecpl,
    feed: true,
    capture: true)

  src += [
    ppc_impl,
    'kpx_cpu/src/mathlib/ieeefp.cpp',
    'kpx_cpu/src/mathlib/mathlib.cpp',
    'kpx_cpu/src/cpu/ppc/ppc-cpu.cpp',
    'kpx_cpu/src/cpu/ppc/ppc-decode.cpp',
    'kpx_cpu/src/cpu/ppc/ppc-execute.cpp',
    'kpx_cpu/src/cpu/ppc/ppc-translate.cpp',
    'kpx_cpu/src/utils/utils-cpuinfo.cpp',
    'kpx_cpu/sheepshaver_glue.cpp',
    'kpx_cpu/ppc-dis.c',
  ]
endif

if enable_mon
  inc_dirs += '../../cxmon/src'
  src += [
    '../../cxmon/src/mon.cpp',
    '../../cxmon/src/mon_6502.cpp',
    '../../cxmon/src/mon_z80.cpp',
    '../../cxmon/src/mon_cmd.cpp',
    '../../cxmon/src/mon_lowmem.cpp',
    '../../cxmon/src/mon_disass.cpp',
    '../../cxmon/src/mon_ppc.cpp',
    '../../cxmon/src/disass/floatformat.c',
    '../../cxmon/src/disass/i386-dis.c',
    '../../cxmon/src/disass/m68k-dis.c',
    '../../cxmon/src/disass/m68k-opc.c',
  ]
endif

if host_system == 'windows'
  windows = import('windows')
  src += windows.compile_resources('Windows/SheepShaver.rc')
  ui_src += windows.compile_resources('Windows/SheepShaverGUI.rc')
  if use_gtk
    executable('SheepShaverGUI', ui_src,
      dependencies: deps,
      include_directories: inc_dirs,
      install: true)
  endif
else
  src += ui_src
endif

executable('SheepShaver', src,
  dependencies: [deps, sdl],
  include_directories: inc_dirs,
  install: true)

install_data(keycodes,
  install_dir: get_option('datadir')/'SheepShaver')

if host_system not in ['windows', 'darwin']
  metainfo_conf = configuration_data()
  metainfo_conf.set('APP_ID', app_id)
  metainfo_conf.set('PACKAGE_URL', package_url)
  metainfo_conf.set('PACKAGE_BUGREPORT', package_bugreport)

  desktop_file = configure_file(
    input: 'Unix/data/SheepShaver.desktop.in',
    configuration: metainfo_conf,
    output: f'@app_id@.desktop')

  install_data(desktop_file,
    install_dir: get_option('datadir') / 'applications')

  metainfo_file = configure_file(
    input: 'Unix/data/SheepShaver.metainfo.xml.in',
    configuration: metainfo_conf,
    output: f'@app_id@.metainfo.xml')

  install_data(metainfo_file,
    install_dir: get_option('datadir') / 'metainfo')

  foreach res: [32, 64, 128, 256]
    install_data(f'Unix/data/SheepShaver@res@.png',
      install_dir: get_option('datadir') / f'icons/hicolor/@res@x@res@/apps',
      rename: f'@app_id@.png')
  endforeach

  install_man('Unix/SheepShaver.1')
endif

