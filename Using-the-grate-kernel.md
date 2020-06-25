# Nexus 7 2012 (grouper)

Download the repository with:
```
$ git clone https://github.com/grate-driver/linux.git
```

## Get the cross-compiler

### Gentoo

```
# crossdev -t armv7a-hardfloat-linux-gnueabi
```

### Ubuntu/Debian

```
# apt-get install gcc-arm-linux-gnueabihf
```

---

You should also be able to use binary toolchains by [Arm](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads), [Linaro](https://www.linaro.org/downloads/) or any other that are targeting `armv7a-hardfloat-linux-gnueabi`, `arm-linux-gnueabi` or `arm-none-linux-gnueabihf` (including those provided by your distribution)

## Set a variable pointing to the toolchain

### Gentoo

```
$ export CROSS_COMPILE_TOOLCHAIN=/usr/bin/armv7a-hardfloat-linux-gnueabi-
```

### Ubuntu/Debian

```
$ export CROSS_COMPILE_TOOLCHAIN=/usr/bin/arm-linux-gnueabihf-
```

### Other distributions/binary toolchains

Look for a directory with files which names end with the following suffixes:

```
-ar
-gcc
-ld
-nm
-objcopy
```

For example, here's a truncated listing of Linaro `gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf` toolchain's `bin` folder:
```
/home/user/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-addr2line
/home/user/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-ar
/home/user/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-as
/home/user/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-c++
/home/user/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-c++filt
/home/user/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-cpp
/home/user/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-dwp
/home/user/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-elfedit
/home/user/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++
/home/user/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
...
```

In this case you can set the variable like so:

```
$ export CROSS_COMPILE_TOOLCHAIN=/home/user/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
```

## Build the kernel

```
$ cd linux  # go to the directory that you cloned the repository to
$ make ARCH=arm CROSS_COMPILE=$CROSS_COMPILE_TOOLCHAIN tegra_defconfig  # set the kernel configuration
$ make ARCH=arm CROSS_COMPILE=$CROSS_COMPILE_TOOLCHAIN -j4  # set -j to your computer's number of threads
```

Once the build is finished, and you see the message `Kernel: arch/arm/boot/zImage is ready`, you need to append .dtb files to the kernel.

In case of Nexus 7 2012 (grouper) there are two that you can append:

```
arch/arm/boot/dts/tegra30-asus-nexus7-grouper-E1565.dtb
arch/arm/boot/dts/tegra30-asus-nexus7-grouper-PM269.dtb
```

These files are created during the compilation.

You can append a device-tree blob like so:

```
$ cat arch/arm/boot/zImage arch/arm/boot/dts/tegra30-asus-nexus7-grouper-E1565.dtb >| zImage-dtb
```

It's more likely that you need the first one. If you get it wrong and the device doesn't boot, just reboot and try the other one.

## Run the freshly-built kernel on your device

The most convenient way is by using fastboot. Reboot your device and hold Vol-. Once you enter fastboot, there should be an image of a green android lying on its back on your device. The device should also show up if you run:

```
$ fastboot devices
```

You can try out the kernel without replacing your current one with:

```
$ fastboot --cmdline "console=tty1" boot zImage-dtb
```

Or you can permanently overwrite the currently installed kernel with:

```
$ fastboot flash boot zImage-dtb
```

And then hit the device's power button to boot it.