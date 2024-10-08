project('BasiliskII', 'c', 'cpp',
  version: '1.1',
  meson_version: '>= 0.64.0',
  default_options: ['buildtype=release', 'cpp_std=gnu++11', 'c_std=gnu11', 'warning_level=1'])

app_id = 'net.cebix.BasiliskII'
package_url = 'https://basilisk.cebix.net'
package_bugreport = 'https://github.com/kanjitalk755/macemu/issues'

cxx = meson.get_compiler('cpp')
host_system = host_machine.system()
build_system = build_machine.system()
config_h = configuration_data()
config_h.set_quoted('APP_ID', app_id)
config_h.set_quoted('PACKAGE_URL', package_url)
config_h.set_quoted('PACKAGE_BUGREPORT', package_bugreport)
args = ['-DHAVE_CONFIG_H=1'] # compile flags for the final executables
link_args = []               # linker flags for the final executables
build_args = []              # compile flags for the build executables
deps = []

if host_system != 'windows'
  data_dir = get_option('datadir') / 'SheepShaver'
  args += f'-DDATADIR="@data_dir@"'
endif

enable_gtk = false
if host_system == 'windows' and get_option('gtk') == 'auto'
  want_gtk = '2'
elif host_system == 'darwin' and get_option('gtk') == 'auto'
  want_gtk = 'disabled'
else
  want_gtk = get_option('gtk')
endif

if want_gtk != 'disabled'
  if want_gtk != '2'
    gtk = dependency('gtk+-3.0', version: '>= 3.22.0', required: want_gtk == '3')
  endif
  if want_gtk == '2' or not gtk.found()
    gtk = dependency('gtk+-2.0', version: '>= 2.24.0')
  endif
  deps += gtk
  enable_gtk = true
  config_h.set('ENABLE_GTK', 1)
endif

have_sdl = false
if get_option('sdl') != 'disabled' or host_system == 'windows'
  config_h.set('USE_SDL', 1)
  config_h.set('USE_SDL_AUDIO', 1)
  config_h.set('USE_SDL_VIDEO', 1)
  if get_option('sdl') != '2'
    sdl = dependency('sdl3', version: '>= 3.0', required: get_option('sdl') == '3')
    if sdl.found()
      have_sdl = true
    endif
  endif
  if not have_sdl
    sdl = dependency('sdl2', version: '>= 2.0.5')
  endif
  have_sdl = true
elif
  deps += [dependency('x11'), dependency('xext')]
endif

if not get_option('bincue').disabled()
  if have_sdl or host_system == 'darwin'
    args += '-DBINCUE'
  elif get_option('bincue').enabled()
    error('BIN/CUE support requires SDL.')
  endif
endif

if get_option('slirp')
  config_h.set('HAVE_SLIRP', 1)
endif

if get_option('jit')
  args += ['-DUSE_JIT=1', '-DUSE_JIT_FPU']
endif

enable_mon = false
if get_option('mon')
  foreach l: ['ncurses', 'termcap', 'termlib', 'terminfo', 'Hcurses', 'curses']
    termlib = cxx.find_library(l, required: false)
    if termlib.found() and cxx.has_function('tgetent', dependencies: termlib)
      deps += termlib
      config_h.set('HAVE_LIB' + l.to_upper(), 1)
      break
    endif
  endforeach

  readline = cxx.find_library('readline', required: false)
  if readline.found()
    deps += readline
    config_h.set('HAVE_LIBREADLINE', 1)
    foreach h: ['readline.h', 'history.h', 'readline/readline.h', 'readline/history.h']
      if cxx.has_header(h)
        config_h.set('HAVE_' + h.underscorify().to_upper(), 1)
      endif
    endforeach
  endif
  if termlib.found() and readline.found()
      config_h.set('ENABLE_MON', 1)
      enable_mon = true
  else
    warning('Terminal libraries not found, disabling mon.')
  endif
endif

libvhd = cxx.find_library('vhd', required: get_option('libvhd'))
libuuid = cxx.find_library('uuid', required: get_option('libvhd'))
if libvhd.found() and libuuid.found()
  deps += [libvhd, libuuid]
endif

if host_machine.cpu_family().startswith('ppc')
  if host_system in ['linux', 'freebsd']
    config_h.set('HAVE_PTHREAD_CANCEL', 1)
    pthread_functions = []
  else
    pthread_functions = ['clock_gettime', 'clock_nanosleep']
  endif
else
  deps += dependency('threads')
  config_h.set('EMULATED_PPC', 1)
  config_h.set('HAVE_PTHREADS', 1)
  pthread_functions = ['clock_gettime', 'clock_nanosleep', 'pthread_cancel',
    'pthread_cond_init', 'pthread_testcancel', 'pthread_mutexattr_setprotocol',
    'pthread_mutexattr_settype', 'pthread_mutexattr_setpshared']
endif

if host_system == 'windows'
  deps += [cxx.find_library('iphlpapi'), cxx.find_library('ws2_32')]
elif host_system == 'darwin'
  add_languages('objc', 'objcpp', native: false)
  deps += dependency('appleframeworks', modules: ['AppKit', 'IOKit', 'CoreFoundation', 'Metal'])
elif host_system == 'sunos'
  deps += [cxx.find_library('nsl'), cxx.find_library('socket')]
endif

headers = [
  'malloc.h',
  'stdint.h',
  'mach/vm_map.h',
  'mach/mach_init.h',
  'sys/mman.h',
  'unistd.h',
  'fcntl.h',
  'byteswap.h',
  'dirent.h',
  'sys/socket.h',
  'sys/ioctl.h',
  'sys/filio.h',
  'sys/bitypes.h',
  'sys/wait.h',
  'sys/time.h',
  'sys/poll.h',
  'sys/select.h',
  'arpa/inet.h',
  'netinet/in.h',
  'linux/if.h',
  'linux/if_tun.h',
  'net/if.h',
  'net/if_tun.h',
  'AvailabilityMacros.h',
  'IOKit/storage/IOBlockStorageDevice.h',
  'fenv.h',
  'sys/stropts.h',
  'stropts.h',
  'strings.h',
  'login.h',
  'sys/bsdtty.h',
  'sys/stat.h',
  'util.h',
  'pty.h',
]

foreach h: headers
  if cxx.has_header(h)
    config_h.set('HAVE_' + h.underscorify().to_upper(), 1)
  endif
endforeach

foreach s: ['short', 'int', 'long', 'long long', 'float', 'double']
  config_h.set('SIZEOF_' + s.underscorify().to_upper(), cxx.sizeof(s))
endforeach
config_h.set('SIZEOF_VOID_P', cxx.sizeof('void *'))

if cxx.has_type('loff_t')
  config_h.set('HAVE_LOFF_T', 1)
endif

if not cxx.has_type('off_t', prefix: '#include <sys/types.h>')
  config_h.set('off_t', 'long int')
endif

if not cxx.has_type('size_t', prefix: '#include <sys/types.h>')
  config_h.set('size_t', 'unsigned int')
endif

functions = ['strdup', 'strerror', 'strlcpy', 'cfmakeraw', 'timer_create',
  'sigaction', 'signal', 'mmap', 'mprotect', 'munmap', 'vm_allocate',
  'vm_deallocate', 'vm_protect', 'poll', 'inet_aton', 'mach_task_self',
  'task_self', '_getpty', 'openpty', 'vhangup', 'isnanl', 'isinfl', 'logl',
  'log10l', 'expl', 'powl', 'fabsl', 'sqrtl', 'sinl', 'cosl', 'tanl', 'sinhl',
  'coshl', 'tanhl', 'asinl', 'acosl', 'atanl', 'asinhl', 'acoshl', 'atanhl',
  'floorl', 'ceill']
functions += pthread_functions

foreach f: functions
  if cxx.has_function(f)
    config_h.set('HAVE_' + f.underscorify().to_upper(), 1)
  endif
endforeach

if config_h.get('HAVE_MMAP', 0) == 1 and \
   config_h.get('HAVE_MPROTECT', 0) == 1 and \
   config_h.get('HAVE_MUNMAP', 0) == 1
  config_h.set('HAVE_MMAP_VM', 1)
endif

if config_h.get('HAVE_VM_ALLOCATE', 0) == 1 and \
   config_h.get('HAVE_VM_DEALLOCATE', 0) == 1 and \
   config_h.get('HAVE_VM_PROTECT', 0) == 1
  config_h.set('HAVE_MACH_VM', 1)
endif

if host_system == 'darwin' and host_machine.cpu() == 'x86_64'
  link_args += ['-Wl,-no_pie', '-pagezero_size', '0x2000']
  config_h.set('PAGEZERO_HACK', 1)
endif

if host_system == 'windows'
  message('Using Win32 exceptions for SIGSEGV recovery')
  can_vosf = true
  config_h.set('HAVE_WIN32_VM', 1)
  config_h.set('HAVE_WIN32_EXCEPTIONS', 1)
  config_h.set('HAVE_SIGSEGV_SKIP_INSTRUCTION', 1)
elif meson.can_run_host_binaries()
  sigsegv_test = ''
  foreach key: config_h.keys()
    value = config_h.get(key, '')
    sigsegv_test += f'#define @key@ @value@\n'
  endforeach
  sigsegv_test += '''
#define @0@ 1
#define CONFIGURE_TEST_SIGSEGV_RECOVERY
#include "vm_alloc.cpp"
#include "sigsegv.cpp"
  '''
  inc_dir = include_directories('src/CrossPlatform')

  if cxx.run(sigsegv_test.format('HAVE_SIGINFO_T'),
    name: 'sigsegv_recovery_siginfo',
    include_directories: inc_dir).returncode() == 0
    config_h.set('HAVE_SIGINFO_T', 1)
    sigsegv_recovery = 'siginfo'
    sigsegv_test = '#define HAVE_SIGINFO_T 1\n' + sigsegv_test
  elif cxx.run(sigsegv_test.format('HAVE_MACH_EXCEPTIONS'),
    name: 'sigsegv_mach_exceptions',
    include_directories: inc_dir).returncode() == 0
    config_h.set('HAVE_MACH_EXCEPTIONS', 1)
    sigsegv_recovery = 'mach'
    sigsegv_test = '#define HAVE_MACH_EXCEPTIONS 1\n' + sigsegv_test
  elif cxx.run(sigsegv_test.format('HAVE_SIGCONTEXT_SUBTERFUGE'),
    name: 'sigsegv_subterfuge',
    include_directories: inc_dir).returncode() == 0
    config_h.set('HAVE_SIGCONTEXT_SUBTERFUGE', 1)
    sigsegv_recovery = 'sigcontext'
    sigsegv_test = '#define HAVE_SIGCONTEXT_SUBTERFUGE 1\n' + sigsegv_test
  else
    sigsegv_recovery = 'None'
  endif

  if sigsegv_recovery != 'None' and \
     cxx.run(sigsegv_test.format('HAVE_SIGSEGV_SKIP_INSTRUCTION'),
       name: 'sigsegv_skip_instruction',
       include_directories: inc_dir).returncode() == 0
    config_h.set('HAVE_SIGSEGV_SKIP_INSTRUCTION', 1)
    can_vosf = true
  else
    can_vosf = false
  endif

elif host_system == 'darwin'
  message('Using Mach SIGSEGV recovery')
  can_vosf = true
  config_h.set('HAVE_MACH_EXCEPTIONS', 1)
  config_h.set('HAVE_SIGSEGV_SKIP_INSTRUCTION', 1)
else
  message('Cross-compiling, assuming siginfo SIGSEGV recovery')
  can_vosf = true
  config_h.set('HAVE_SIGINFO_T', 1)
  config_h.set('HAVE_SIGSEGV_SKIP_INSTRUCTION', 1)
endif

if can_vosf
  if get_option('vosf') and host_system != 'darwin'
    config_h.set('ENABLE_VOSF', 1)
  endif
  if get_option('addressing') == 'real'
    args += '-DREAL_ADDRESSING'
  elif get_option('addressing') == 'direct'
    args += '-DDIRECT_ADDRESSING'
  endif
endif

if cxx.has_member('struct sigaction', 'sa_restorer',
     prefix: '#include <signal.h>')
  config_h.set('HAVE_SIGNAL_SA_RESTORER', 1)
endif

config_h.set('HOST_FLOAT_FORMAT', 'IEEE_FLOAT_FORMAT')
if host_machine.endian() == 'big'
  config_h.set('HOST_FLOAT_WORDS_BIG_ENDIAN', 1)
else
  config_h.set('HOST_FLOAT_WORDS_BIG_ENDIAN', 0)
endif

subdir('src')

