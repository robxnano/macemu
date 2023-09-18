project('SheepShaver', 'c', 'cpp',
  version: '2.5.1',
  meson_version: '>= 0.50.0',
  default_options: ['buildtype=release', 'cpp_std=gnu++11', 'c_std=gnu11', 'warning_level=1'])

add_project_arguments('-DHAVE_CONFIG_H=1', '-D_REENTRANT',
  '-DDATADIR="@0@/SheepShaver"'.format(get_option('datadir')),
  language: ['c', 'cpp'])

cxx = meson.get_compiler('cpp')
host_system = host_machine.system()
config_h = configuration_data()
sheepshaver_deps = []

if not get_option('gtk').disabled()
  config_h.set('ENABLE_GTK', 1)
  sheepshaver_deps += dependency('gtk+-3.0', version: '>= 3.22.0')
  add_project_link_arguments('-rdynamic', language: ['c', 'cpp'])
endif
if not get_option('sdl').disabled()
  config_h.set('USE_SDL', 1)
  config_h.set('USE_SDL_AUDIO', 1)
  config_h.set('USE_SDL_VIDEO', 1)
  sheepshaver_deps += [
    dependency('sdl2', version: '>= 2.0.5'),
    dependency('x11'),
    dependency('xext')]
endif
if not (get_option('bincue').disabled() or get_option('sdl').disabled())
  add_project_arguments('-DBINCUE', language: ['c', 'cpp'])
endif
if not get_option('slirp').disabled()
  config_h.set('HAVE_SLIRP', 1)
endif
if not get_option('jit').disabled()
  config_h.set('USE_JIT', 1)
endif

if host_machine.cpu_family().startswith('ppc')
  if host_system in ['linux', 'freebsd']
    config_h.set('HAVE_PTHREAD_CANCEL', 1)
    pthread_functions = []
  else
    pthread_functions = ['clock_gettime', 'clock_nanosleep']
  endif
else
  sheepshaver_deps += dependency('threads')
  config_h.set('EMULATED_PPC', 1)
  config_h.set('HAVE_PTHREADS', 1)
  pthread_functions = ['clock_gettime', 'clock_nanosleep', 'pthread_cancel',
    'pthread_cond_init', 'pthread_testcancel', 'pthread_mutexattr_setprotocol',
    'pthread_mutexattr_settype', 'pthread_mutexattr_setpshared']
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

functions = ['strdup', 'strerror', 'strlcpy', 'cfmakeraw', 'nanosleep',
  'sigaction', 'signal', 'mmap', 'mprotect', 'munmap', 'vm_allocate',
  'vm_deallocate', 'vm_protect', 'exp2f', 'log2f', 'exp2', 'log2', 'floorf',
  'roundf', 'ceilf', 'truncf', 'floor', 'round', 'ceil', 'trunc', 'poll',
  'inet_aton', 'mach_task_self', 'task_self']
functions += pthread_functions

foreach f: functions
  if cxx.has_function(f)
    config_h.set('HAVE_' + f.underscorify().to_upper(), 1)
  endif
endforeach

if get_option('mon').enabled()
  foreach l: ['ncurses', 'termcap', 'termlib', 'terminfo', 'Hcurses', 'curses']
    termlib = cxx.find_library(l, required: false)
    if termlib.found() and cxx.has_function('tgetent', dependencies: termlib)
      sheepshaver_deps += termlib
      config_h.set('HAVE_LIB' + l.to_upper(), 1)
      break
    endif
  endforeach

  readline = cxx.find_library('readline', required: false)
  if readline.found()
    sheepshaver_deps += readline
    config_h.set('HAVE_LIBREADLINE', 1)
    foreach h: ['readline.h', 'history.h', 'readline/readline.h', 'readline/history.h']
      if cxx.has_header(h)
        config_h.set('HAVE_' + h.underscorify().to_upper(), 1)
      endif
    endforeach
  endif
  if termlib.found() and readline.found()
      config_h.set('ENABLE_MON', 1)
  else
    warning('Terminal libraries not found, disabling mon.')
  endif
endif

if config_h.get('HAVE_MMAP') == 1 and config_h.get('HAVE_MPROTECT') == 1 and config_h.get('HAVE_MUNMAP') == 1
  config_h.set('HAVE_MMAP_VM', 1)
endif

conftest = configuration_data()
conftest.merge_from(config_h)
test_file = configure_file(output: 'conftest.h', configuration: conftest)
test_str = run_command(['cat', test_file], capture: true, check: false).stdout()
sigsegv_test = test_str + '''
#define @0@ 1
#define CONFIGURE_TEST_SIGSEGV_RECOVERY
#include "vm_alloc.cpp"
#include "sigsegv.cpp"
'''
inc_dir = include_directories('src/CrossPlatform')
if cxx.run(sigsegv_test.format('HAVE_SIGINFO_T'),
  name: 'sigsegv_recovery',
  include_directories: inc_dir).returncode() == 0
  config_h.set('HAVE_SIGINFO_T', 1)
  config_h.set('ENABLE_VOSF', 1)
  sigsegv_recovery = 'siginfo'
  sigsegv_test = '#define HAVE_SIGINFO_T 1\n' + sigsegv_test
elif cxx.run(sigsegv_test.format('HAVE_SIGCONTEXT_SUBTERFUGE'),
  name: 'sigsegv_subterfuge',
  include_directories: inc_dir).returncode() == 0
  config_h.set('HAVE_SIGCONTEXT_SUBTERFUGE', 1)
  config_h.set('ENABLE_VOSF', 1)
  sigsegv_recovery = 'sigcontext'
  sigsegv_test = '#define HAVE_SIGCONTEXT_SUBTERFUGE 1\n' + sigsegv_test
else
  sigsegv_recovery = 'None'
endif

if cxx.run(sigsegv_test.format('HAVE_SIGSEGV_SKIP_INSTRUCTION'),
  name: 'sigsegv_skip_instruction',
  include_directories: inc_dir).returncode() == 0
  config_h.set('HAVE_SIGSEGV_SKIP_INSTRUCTION', 1)
  config_h.set('ENABLE_VOSF', 1)
endif

config_h.set('HAVE_SIGNAL_SA_RESTORER',
  cxx.has_member('struct sigaction', 'sa_restorer',
    prefix: '#include <signal.h>'))

use_dyngen = get_option('dyngen')
if use_dyngen == 'auto'
  if host_machine.cpu_family().startswith('ppc')
    use_dyngen = 'no'
  else
    use_dyngen ='precompiled'
  endif
endif

subdir('src')