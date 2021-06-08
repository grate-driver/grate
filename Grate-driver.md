# Grate-driver packages

# Important

:exclamation: The GRATE drivers are for older NVIDIA Tegra20/30/114 SoCs only! If you're looking for drivers for TK1/TX1, then you should use [Nouveau drivers](https://nouveau.freedesktop.org/wiki/). 
***

### Arch Linux AUR

[https://aur.archlinux.org/packages/?K=aa13q&SeB=m](https://aur.archlinux.org/packages/?K=aa13q&SeB=m)

### Gentoo overlay

[https://github.com/grate-driver/grate-overlay](https://github.com/grate-driver/grate-overlay)

### Ubuntu PPA

[https://code.launchpad.net/~grate-driver/+archive/ubuntu/ppa](https://code.launchpad.net/~grate-driver/+archive/ubuntu/ppa)

### Fedora repository

[https://copr.fedorainfracloud.org/coprs/kwizart/grate-driver/](https://copr.fedorainfracloud.org/coprs/kwizart/grate-driver/)

### PostmarketOS

[https://wiki.postmarketos.org/wiki/Nvidia_Tegra_2_(tegra20)](https://wiki.postmarketos.org/wiki/Nvidia_Tegra_2_(tegra20))

[https://wiki.postmarketos.org/wiki/Nvidia_Tegra_3_(tegra30)](https://wiki.postmarketos.org/wiki/Nvidia_Tegra_3_(tegra30))

# Build it yourself

## General rules:

  1. Use the master branches.
  2. Install autotools and gcc.
  3. Compile and install libdrm first.
  4. Use the most recent [mainline linux kernel](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/) or at least the most recent [stable](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git/). You may also try the [experimental grate kernel](https://github.com/grate-driver/linux).
  5. Update all `libdrm` `opentegra` `mesa` `libvdpau-tegra` at once as there could be interdependencies, start from libdrm.

### Libdrm:

  1. `git clone https://github.com/grate-driver/libdrm.git`
  2. `cd libdrm`
  3. `sh autogen.sh --prefix=/usr --enable-tegra-experimental-api --disable-freedreno --disable-vmwgfx --disable-nouveau --disable-amdgpu --disable-radeon --disable-intel --disable-vc4`
  4. `make install`

### Opentegra:

  1. `git clone https://github.com/grate-driver/xf86-video-opentegra.git`
  2. `cd xf86-video-opentegra`
  3. `sh autogen.sh --prefix=/usr`
  4. `make install`

### Software GL:

Note that Display Managers may require GL support, but it's not available yet and automatic fallback doesn't work with Opentegra. Add these lines to `/etc/security/pam_env.conf` to fix this problem:

```
QT_QUICK_BACKEND=software
LIBGL_ALWAYS_SOFTWARE=1
```

This will provide a system-wide fall back to software GL.

By default the `llvmpipe` Mesa driver will be used and it doesn't work on Tegra20 or if NEON is disabled. In this case use `softpipe` driver:

```
GALLIUM_DRIVER=softpipe
```

### Mesa:

  1. `git clone https://github.com/grate-driver/mesa.git`
  2. `cd mesa`
  3. `git checkout 21.1.0`
  4. `meson -Dgallium-drivers=grate,swrast -Dplatforms=x11 -Dshared-glapi=true -Dgbm=true -Dglx=dri  -Dgles1=false -Dgles2=true -Degl=true -Dgallium-xa=false -Dgallium-vdpau=false -Dgallium-va=false -Dgallium-xvmc=false -Duse-elf-tls=false -Dgallium-nine=false -Db_ndebug=true -Dvulkan-drivers= -Dlibunwind=false -Dllvm=false build`
  5. `cd build/`
  6. `ninja && ninja install`

#### Example cross-file for meson:

```
[binaries]
ar = ['armv7a-hardfloat-linux-gnueabi-ar']
c = ['armv7a-hardfloat-linux-gnueabi-gcc']
cpp = ['armv7a-hardfloat-linux-gnueabi-g++']
fortran = ['gfortran']
llvm-config = 'llvm-config'
objc = ['cc']
objcpp = ['armv7a-hardfloat-linux-gnueabi-c++']
pkgconfig = 'armv7a-hardfloat-linux-gnueabi-pkg-config'
strip = ['armv7a-hardfloat-linux-gnueabi-strip']
windres = ['windres']

[properties]
c_args = ['-O2', '-pipe', '-g', '-fPIC', '-mcpu=cortex-a9', '-mfpu=vfpv3-d16', '-mfloat-abi=hard']
c_link_args = ['-O2', '-pipe', '-g', '-fPIC', '-mcpu=cortex-a9', '-mfpu=vfpv3-d16', '-mfloat-abi=hard', '-L/usr/armv7a-hardfloat-linux-gnueabi/', '-L/usr/armv7a-hardfloat-linux-gnueabi/lib', '-L/usr/armv7a-hardfloat-linux-gnueabi/usr/lib', '-Wl,-O1', '-Wl,--as-needed']
cpp_args = ['-O2', '-pipe', '-g', '-fPIC', '-mcpu=cortex-a9', '-mfpu=vfpv3-d16', '-mfloat-abi=hard']
cpp_link_args = ['-O2', '-pipe', '-g', '-fPIC', '-mcpu=cortex-a9', '-mfpu=vfpv3-d16', '-mfloat-abi=hard', '-L/usr/armv7a-hardfloat-linux-gnueabi/', '-L/usr/armv7a-hardfloat-linux-gnueabi/lib', '-L/usr/armv7a-hardfloat-linux-gnueabi/usr/lib', '-Wl,-O1', '-Wl,--as-needed']
fortran_args = ['-O2', '-pipe', '-march=armv7-a']
fortran_link_args = ['-O2', '-pipe', '-march=armv7-a', '-L/usr/armv7a-hardfloat-linux-gnueabi/', '-L/usr/armv7a-hardfloat-linux-gnueabi/lib', '-L/usr/armv7a-hardfloat-linux-gnueabi/usr/lib', '-Wl,-O1', '-Wl,--as-needed']
objc_args = []
objc_link_args = ['-L/usr/armv7a-hardfloat-linux-gnueabi/', '-L/usr/armv7a-hardfloat-linux-gnueabi/lib', '-L/usr/armv7a-hardfloat-linux-gnueabi/usr/lib', '-Wl,-O1', '-Wl,--as-needed']
objcpp_args = []
objcpp_link_args = ['-L/usr/armv7a-hardfloat-linux-gnueabi/', '-L/usr/armv7a-hardfloat-linux-gnueabi/lib', '-L/usr/armv7a-hardfloat-linux-gnueabi/usr/lib', '-Wl,-O1', '-Wl,--as-needed']

[host_machine]
system = 'linux'
cpu_family = 'arm'
cpu = 'armv7a'
endian = 'little'
```

Outdated instructions: 
  1. **for older Mesa 19 and older versions only!**
  2. `git clone https://github.com/grate-driver/mesa.git`
  3. `cd mesa`
  4. `meson -Dprefix=/usr -Dgallium-drivers=grate -Ddri-drivers=swrast -Dplatforms=x11,drm -Dshared-glapi=true -Dgbm=true -Dglx=dri -Dosmesa=none -Dgles1=false -Dgles2=true -Degl=true -Dgallium-xa=false -Dgallium-vdpau=false -Dgallium-va=false -Dgallium-xvmc=false -Duse-elf-tls=false -Dgallium-nine=false -Db_ndebug=true -Dvulkan-drivers= -Dlibunwind=false -Dllvm=false build`
  5. `cd build/`
  6. `ninja && ninja install`

Outdated instructions: 
  1. **for older Mesa versions only!**
  2. `git clone https://github.com/grate-driver/mesa.git`
  3. `cd mesa`
  4. `sh autogen.sh --prefix=/usr --enable-dri --enable-glx --enable-shared-glapi --enable-texture-float --disable-nine --enable-debug --enable-dri3 --enable-egl --enable-gbm --enable-gles1 --enable-gles2 --enable-glx-tls --enable-valgrind=auto --enable-llvm-shared-libs --with-dri-drivers=swrast --with-gallium-drivers=swrast,grate --with-vulkan-drivers= --with-egl-platforms=x11,drm --disable-nine --disable-llvm --disable-omx-bellagio --disable-va --disable-vdpau --disable-xa --disable-xvmc --disable-gallium-osmesa --disable-libunwind`
  5. `make install`

### Libvdpau-tegra:

  1. `git clone https://github.com/grate-driver/libvdpau-tegra.git`
  2. `cd libvdpau-tegra`
  3. `sh autogen.sh --prefix=/usr`
  4. `make install`
  5. `udevadm trigger`