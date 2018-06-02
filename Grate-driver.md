# Grate-driver packages

### Ubuntu PPA

[https://code.launchpad.net/~grate-driver/+archive/ubuntu/ppa](https://code.launchpad.net/~grate-driver/+archive/ubuntu/ppa)

### Gentoo overlay

[https://github.com/grate-driver/grate-overlay](https://github.com/grate-driver/grate-overlay)

### Arch Linux AUR

[https://aur.archlinux.org/packages/?K=aa13q&SeB=m](https://aur.archlinux.org/packages/?K=aa13q&SeB=m)

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

### Mesa:

  1. `git clone https://github.com/grate-driver/mesa.git`
  2. `cd mesa`
  3. `sh autogen.sh --prefix=/usr --enable-dri --enable-glx --enable-shared-glapi --enable-texture-float --disable-nine --enable-debug --enable-dri3 --enable-egl --enable-gbm --enable-gles1 --enable-gles2 --enable-glx-tls --enable-valgrind=auto --enable-llvm-shared-libs --with-dri-drivers=swrast --with-gallium-drivers=swrast,grate --with-vulkan-drivers= --with-egl-platforms=x11,drm --disable-nine --disable-llvm --disable-omx-bellagio --disable-va --disable-vdpau --disable-xa --disable-xvmc --disable-gallium-osmesa --disable-libunwind`
  4. `make install`

### Libvdpau-tegra:

  1. `git clone https://github.com/grate-driver/libvdpau-tegra.git`
  2. `cd libvdpau-tegra`
  3. `sh autogen.sh --prefix=/usr`
  4. `make install`