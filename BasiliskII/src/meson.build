configure_file(output: 'config.h', configuration: config_h)

if host_machine.cpu_family() in ['arm', 'arm64']
  uae_path = 'uae_cpu_2021'
  fpu_src = uae_path / 'fpu/fpu_mpfr.cpp'
  args += '-DFPU_MPFR'
  deps += [cxx.find_library('mpfr'), cxx.find_library('gmp')]
else
  uae_path = 'uae_cpu'
  fpu_src = uae_path / 'fpu/fpu_ieee.cpp'
  args += '-DFPU_IEEE'
endif

inc_dirs = ['include', uae_path, 'CrossPlatform']
build_inc_dirs = inc_dirs
inc_dirs += host_system == 'windows' ? 'Windows' : 'Unix'
build_inc_dirs += build_system == 'windows' ? 'Windows' : 'Unix'
if host_system == 'darwin'
  inc_dirs += 'MacOSX'
endif

if host_machine.cpu_family() == 'x86_64'
  args += ['-DX86_64_ASSEMBLY', '-DUNALIGNED_PROFITABLE', '-DOPTIMIZED_FLAGS']
elif host_machine.cpu_family() == 'x86'
  args += ['-DX86_ASSEMBLY', '-DSAHF_SETO_PROFITABLE', '-DOPTIMIZED_FLAGS']
endif

if build_machine.cpu_family() == 'x86_64'
  build_args += ['-DX86_64_ASSEMBLY', '-DUNALIGNED_PROFITABLE', '-DOPTIMIZED_FLAGS']
elif build_machine.cpu_family() == 'x86'
  build_args += ['-DX86_ASSEMBLY', '-DSAHF_SETO_PROFITABLE', '-DOPTIMIZED_FLAGS']
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
  'macos_util.cpp',
  'main.cpp',
  'prefs.cpp',
  'prefs_items.cpp',
  'rom_patches.cpp',
  'rsrc_patches.cpp',
  'scsi.cpp',
  'serial.cpp',
  'slot_rom.cpp',
  'sony.cpp',
  'timer.cpp',
  'user_strings.cpp',
  'video.cpp',
  'xpram.cpp',
  'CrossPlatform/vm_alloc.cpp',
  'CrossPlatform/sigsegv.cpp',
  'CrossPlatform/video_blit.cpp',
  uae_path / 'basilisk_glue.cpp',
  uae_path / 'compiler/compemu_support.cpp',
  uae_path / 'memory.cpp',
  uae_path / 'newcpu.cpp',
  uae_path / 'readcpu.cpp',
  fpu_src,
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

build68k = executable('build68k', uae_path / 'build68k.c',
  native: true,
  c_args: build_args,
  include_directories: build_inc_dirs)

build68k_cmd = build_system == 'windows' ? [build68k, '@INPUT@'] : build68k
cpudefs = custom_target('cpudefs',
  output: 'cpudefs.cpp',
  input: uae_path / 'table68k',
  command: build68k_cmd,
  feed: build_system != 'windows',
  capture: true)

gencpu = executable('gencpu', uae_path / 'gencpu.c', uae_path / 'readcpu.cpp', cpudefs,
  native: true,
  c_args: build_args,
  cpp_args: build_args,
  build_by_default: true,
  include_directories: build_inc_dirs)

gencomp = executable('gencomp', uae_path / 'compiler/gencomp.c', uae_path / 'readcpu.cpp', cpudefs,
  native: true,
  c_args: build_args,
  cpp_args: build_args,
  build_by_default: true,
  include_directories: build_inc_dirs)

cpuemu = custom_target('cpuemu',
  output: ['cpuemu.cpp', 'cpuemu_nf.cpp', 'cpustbl.cpp', 'cpustbl_nf.cpp', 'cputbl.h'],
  command: ['uae_cpu/wrapper.py', gencpu, '@OUTDIR@'])

gencomp_out = custom_target('compemu',
  output: ['compemu.cpp', 'compstbl.cpp', 'comptbl.h'],
  command: ['uae_cpu/wrapper.py', gencomp, '@OUTDIR@'])

compemu = gencomp_out[0]
compstbl = gencomp_out[1]
comptbl_h = gencomp_out[2]

src += [cpudefs, comptbl_h, compstbl]

if enable_gtk
  if gtk.version().version_compare('>= 3.0')
    gnome = import('gnome')
    if cxx.has_link_argument('-rdynamic')
      link_args += '-rdynamic'
    endif
    ui_xml = files(
      'Unix/ui/help-overlay.ui',
      'Unix/ui/menu.ui',
      'Unix/ui/prefs-editor.ui',
    )
    ui = gnome.compile_resources('g_resource', 'Unix/ui/basiliskii.gresource.xml',
      c_name: 'basiliskii',
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
      'Windows/b2ether/packet32.cpp',
    ]
  else
    ui_src += 'Unix/prefs_editor_gtk.cpp'
  endif
else
  src += 'dummy/prefs_editor_dummy.cpp'
endif

if get_option('slirp')
  args += cxx.get_supported_arguments('-fno-strict-aliasing')
  inc_dirs += 'slirp'
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

exe_deps = deps
if have_sdl
  exe_deps += sdl
  src += [
    'SDL/video_sdl2.cpp',
    'SDL/video_sdl3.cpp',
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

cpuemus = []
foreach i: range(1, 9)
  cpuemus += static_library(f'cpuemu@i@', cpuemu,
    cpp_args: [args, f'-DPART_@i@'],
    include_directories: inc_dirs)
  cpuemus += static_library(f'cpuemu@i@_nf', cpuemu,
    cpp_args: [args, f'-DPART_@i@', '-DNOFLAGS'],
    include_directories: inc_dirs)
  cpuemus += static_library(f'compemu@i@', compemu,
    cpp_args: [args, f'-DPART_@i@'],
    include_directories: inc_dirs)
endforeach

if host_system == 'windows'
  windows = import('windows')
  src += windows.compile_resources('Windows/BasiliskII.rc')
  ui_src += windows.compile_resources('Windows/BasiliskIIGUI.rc')
  if enable_gtk
    executable('BasiliskIIGUI', ui_src,
      dependencies: deps,
      c_args: args,
      cpp_args: args,
      link_args: link_args,
      include_directories: inc_dirs,
      install: true)
  endif
else
  src += ui_src
endif

executable('BasiliskII',
  sources: src,
  c_args: args,
  cpp_args: args,
  link_args: link_args,
  dependencies: exe_deps,
  link_with: cpuemus,
  include_directories: inc_dirs,
  install: true)

install_data(keycodes,
  install_dir: get_option('datadir') / 'BasiliskII')

if host_system not in ['windows', 'darwin']
  metainfo_conf = configuration_data()
  metainfo_conf.set('APP_ID', app_id)
  metainfo_conf.set('PACKAGE_URL', package_url)
  metainfo_conf.set('PACKAGE_BUGREPORT', package_bugreport)

  desktop_file = configure_file(
    input: 'Unix/data/BasiliskII.desktop.in',
    configuration: metainfo_conf,
    output: f'@app_id@.desktop')

  install_data(desktop_file,
    install_dir: get_option('datadir') / 'applications')

  metainfo_file = configure_file(
    input: 'Unix/data/BasiliskII.metainfo.xml.in',
    configuration: metainfo_conf,
    output: f'@app_id@.metainfo.xml')

  install_data(metainfo_file,
    install_dir: get_option('datadir') / 'metainfo')

  foreach res: [64, 128]
    install_data(f'Unix/data/BasiliskII@res@.png',
      install_dir: get_option('datadir') / f'icons/hicolor/@res@x@res@/apps',
      rename: f'@app_id@.png')
  endforeach

  install_man('Unix/BasiliskII.1')
endif

