#### BasiliskII
```
macOS     x86_64 JIT / arm64 non-JIT
Linux x86 x86_64 JIT / arm64 non-JIT
MinGW x86        JIT
```
#### SheepShaver
```
macOS     x86_64 JIT / arm64 non-JIT
Linux x86 x86_64 JIT / arm64 non-JIT
MinGW x86        JIT
```
### How To Build
These builds need SDL 2.0.5+ framework/library to be installed.
Alternatively, SDL 1.2 can be used, but with fewer features.

https://www.libsdl.org
#### BasiliskII
##### macOS
preparation:

Download gmp-6.2.1.tar.xz from https://gmplib.org.
```
$ cd ~/Downloads
$ tar xf gmp-6.2.1.tar.xz
$ cd gmp-6.2.1
$ ./configure --disable-shared
$ make
$ make check
$ sudo make install
```
Download mpfr-4.1.0.tar.xz from https://www.mpfr.org.
```
$ cd ~/Downloads
$ tar xf mpfr-4.1.0.tar.xz
$ cd mpfr-4.1.0
$ ./configure --disable-shared
$ make
$ make check
$ sudo make install
```
On an Apple Silicon Mac, change the `configure` command for both GMP and MPFR as follows, and ignore the `make check` command:
```
CFLAGS="-arch arm64" CXXFLAGS="$CFLAGS" ./configure -host=aarch64-apple-darwin --disable-shared 
```
(from https://github.com/kanjitalk755/macemu/pull/96)

build:
```
$ cd macemu/BasiliskII/src/MacOSX
$ xcodebuild build -project BasiliskII.xcodeproj -configuration Release
```
or same as Linux

##### Linux
Required libraries:
Debian/Ubuntu: build-essential libgtk-3-dev libsdl2-dev
Red Hat/Fedora/SUSE: gcc-c++ make automake pkgconf gtk3-devel SDL2-devel
arm64 only: libgmp-dev libmpfr-dev / gmp-devel mpfr-devel
```
$ cd macemu/BasiliskII/src/Unix
$ ./autogen.sh
$ make
```
Other Unix systems such as FreeBSD may also work.
##### MinGW32/MSYS2
```
$ cd macemu/BasiliskII/src/Windows
$ ../Unix/autogen.sh
$ make
```
#### SheepShaver
##### macOS
```
$ cd macemu/SheepShaver/src/MacOSX
$ xcodebuild build -project SheepShaver_Xcode8.xcodeproj -configuration Release
```
or same as Linux

##### Linux
```
$ cd macemu/SheepShaver/src/Unix
$ ./autogen.sh
$ make
```
##### MinGW32/MSYS2
```
$ cd macemu/SheepShaver
$ make links
$ cd src/Windows
$ ../Unix/autogen.sh
$ make
```
### Recommended key bindings for gnome
https://github.com/kanjitalk755/macemu/blob/master/SheepShaver/doc/Linux/gnome_keybindings.txt

(from https://github.com/kanjitalk755/macemu/issues/59)
