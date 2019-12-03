
# Chapter 1 - Building the System

The Linux kernel together with GNU software and many other open-source components provies a
completely free operating system, GNU/Linux. Embedded Linux is the usage of the Linux kernel
and various open-source components in embedded systems.

Embedded Linux is used in embedded systems such as consumer electronics (e.g., set-top boxes,
smart TVs, PVRs (personal video recordes), IVI (in-vehicle infotainment), networking equipment
(such as routers, switches, WAPs (wireless access points) or wireless routers), machine control,
industrial automation, navigation equipment, spacecraft flight software, and medical instruments
in general).

The main components of an embedded Linux syste:

- Bootloader
- Kernel
- System call interface
- C-Runtime library
- System shared libraries
- Root filesystem

## Bootloader

Linux cannot be started in an embedded device without a small amount of machine specific code to
initialize the system. Linux requires the bootloader code to perform the following tasks:

- Configuration of the memory system.
- Loading of the kernel image and the device tree at the correct addresses.
- Optional loading of an initial RAM disk at the correct memory address.
- Setting of the kernel command-line and other parameters (e.g, device tree, machine type).
- Initialize the serial console.

`U-boot` is the preferred bootloader for Embedded Linux.

# Linux Kernel

Linux kernel is inspire by the Unix and Minix operating system. It aims towards POSIX and Single
UNIX Specification compliance.

It has all the features you would expect in a moderm fully-fledge Unix implementation, including
true multitasking, virtual memory, shared libraries, demand loading, shared copy-on-write
executables, proper memory management, and multistack networking including IPv4 and IPv6.
Althouth originally developed for 32-bit x86-based PCs (386 or higher), today Linux also runs on a
multitude of other processor architectures, in both 32-bit and 64-bit variants.

Some of the subsystems the kernel is comprise of are listed below:

- /arch/<arch>: Architecture specific code
- /arch/<arch>/<match>: Machine/board specific code
- /Documentation: Kernel documentation
- /ipc: Inter process communication
- /mm: Memory management
- /fs: File systems
- /include: Kernel headers
- /include/asm-<arch>: Architecture and machine dependent headers
- /include/linux: Linux kernel code headers
- /init: Linux initialization (including main.c)
- /block: Kernel block layer code
- /net: Networking functionality
- /lib: Common kernel helper functions
- /kernel: Common kernel structures
- /arch: Architecture specific doe
- /security: Security components
- /drivers: Built-in drivers (does not include loadable modules)
- Makefile: Top Linux makefile (set arch and version)
- /scripts: Scripts for internal or external use.

There are several main categories into which kernel releases may fall:

1. Prepatch
2. Mainline
3. Stable
4. Longterm

As kernels move from the `mainline` into the `stable` category, two things can happen:

1. Kernels can reach `End of Life (EOL)` after a few bugfix revisions, which means that kernel
maintainers will release no more bugfixes for this kernel version.
2. Kernels can put into `longterm` maintenance, which means that maintainers will provide
bugfixes for this kernel revision for a much longer period of time.


## System call interface and C runtime library

The system call is the fundamental interface beetween an application and the Linux kernel. System
calls are the only means by which an user space application can interact with the kernel. In other
words, they are the bridge between user space and kernel space. The strict separation of user and
kernel space ensures that user space programs cannot freely access kernel internal resources,
thereby ensuring the security and stability of the system. The system calls elevate the privilege
of the user process.

The `C runtime library (C-standard library) defines macros, type definitions and functions for string
handling, mathematical functions, input/output processing, memory allocation and several other
functions that rely on OS services. The runtime library provides applications with access to OS resources
and functions by abstracting the OS System call interface.

Several C runtime libraries are available: glibc, uClic, eglibc, dietlibc, newlib. The choice of the
C library must be made at the time of cross-compiling toolchain generation, as the GCC compiler
is compiled agains a specific C library.

The GNU C library, `glibc` is the default C library used for example in the Yocto project. You can
find the glibc manual at https://www.gnu.org/software/libc/manual/.


## System Shared Libraries

System shared libraries are libraries that are loaded by programs when they start. When a shared library is
installed properly, all programs that start afterwards automatically use the new shared library. System
shared libraries are typically linked with an user space applciation to provide it access to a specific
system functionality. This system functionality can be either self-contained like compression or encryption
algorithms or require access to underlying kernel resources or hardware. In the latter case the library
provides a simple API that abstracts the complexities of the kernel or direct driver access.

The following system shared libraries are required by the `LSB (Linux Standard Base)` specification and
therefore must be available on all LSB compliant systems:

- Libc: Standard C library (C runtime libary). Elementary language support and OS platform services. Direct
access to the OS System Call Interface.
- Libm: Math library. Common elementary mathematical functions and floating point environment routines
defined by System V, ANSI C, POSIX, etc.
- Libpthread: POSIS thread library. Functionality now in libc, maintained to provide backwards compatibility.
- Libdl: Dynamic linker library. Functionality now in libc, maintained to provide backwards compatibility.
- Libcrypt: Cryptology library. Encryption and decryption handling routines.
- Libpam: PAM (Pluggable Authentication Module) library. Routines for the PAM.
- Libz: Compression/decompression library. General purpose data compression and deflation functionality.
- Libncurses: CRT screen handling and optimization package. Overal screen, window and pad manipulation; output
to windows and pads; reading terminal input; control over terminal and cursor input and output options;
environment query routines; color manipulation; use of soft label keys.
- Libutil; System utilities library. Various system-dependent utility routines used in a wide variety of system
daemons. The abstracted functions are mostly related to pseudo-terminals and login accounting.

Libraries are placed in the following standard root filesystem locations:

- /lib: Libraries required for startup
- /usr/lib: Most system libraries
- /usr/local/lib: Non-system libraries

## Root Filesystem

The root filesystem is where all the files contained int the file hierachy (including device nodes)
are stored. The root filesystem is mounted as /, containing all the libraries, applications and data.

The folder structure of the root filesystem is defined by FHS (Filesystem Hierarchy Standard). The FHS
defines the names, locations, and permissions for many file types and directories. It thereby ensures
compatibility between different Linux distributions and allows applications to make assumptions about
where to find specific system files and configurations.

An embedded Linux root filesystem usually includes the following:

- /bin: Commands needed during bootup that might be used by normal users.
- /sbin: Like /bin, but the commands are not intended for normal users, although they may use them
if necessary and allowed. /sbin is not usually in the default path of normal users, but will be in
root's default path.
- /etc: Configuration files specific to the machine.
- /home: Like My Documents in Windows.
- /root: The home directory for user root. This is usually not accessible to other users on the
system.
- /lib: Essential shared libraries and kernel modules.
- /dev: Device files. These are special virtual files that help the user interface with the varios
devices on the system.
- /tmp: Temporary files. As the name suggests, programs running often store temporary files in here.
- /boot: Files used by the bootstrap loader. Kernel images are often kept here instead of in the
root directory. If there are many kernel images, the directory can easily grow too large and it
might be better to keep it in a separate filesystem.
- /mnt: Mount point for mounting a filesystem temporaly.
- /opt: Add on application software packages.
- /usr: Secondary hierarchy.
- /var: Variable data.
- /sys: Exports information about devices and drivers from the kernel device model to user space,
and is also used for configuration.
- /proc: Represent the current state of the kernel.

## Linux Boot Process

These are the main stages of an embedded Linux boot process:

1. The boot process begins at POR (Power On Reset) where the hardware reset logic forces the ARM
core to begin execution starting from the on-chip boot ROM. The boot ROM can support several devices
(e.g. NOR flash, NAND flash, SD/eMMC). In the i.MX7D processor the on-chip boot ROOM sets up the DDR
memory controller. The DDR technology is a potential key difference between different boards. If
there is a difference in the DDR technology, the DDR initialization needs to be ported. In the
i.MX7D the DDR initialization is coded in the DCD table, inside the boot header of the U-Boot image.
The DCD (device configuration data) feature allows boot ROM code to obtain SoC configuration data
from an external bootloader residing on the boot device. As an example, DCD can be used to program
the DDR controller for optimal settings improving the boot performance. After setting up the DDR
controlloer the boot ROM loads the U-Boot image to external DDR and runs it.

2. The U-Boot loads both the kernel image and the compiled device tree binary into RAM and passes
the memory address of the device tree binary into the kernel as part of the lauch.

3. The U-Boot jumps to the kernel code.

4. Kernel runs low level kernel initialization, enabling MMU and creating the initial table of memory
pages, and setting up caches. This is done in `arch/arm/kernel/head.s`. The file head.s contains
CPU architecture specific but platform independent initialization code. Then the system switches
to the non architecture specific kernel startup function `start_kernel()`.

5. Kernel runs `start_kernel()` located in `init/main.c` that:

- Initializes the kernel code (e.g., memory, scheduling, interrupts, ...).
- Initializes statically compiled drivers.
- Mounts the root filesystem based on bootargs passed to the kernel from U-Boot.
- Executes the first user process, init. The process init, by default, is /init for initramfs, and
/sbin/init for a regular filesystem. The three init programs that you usually find on embedded Linux
devices are `BusyBox init`, `System V init` and `systemd`. If System V is used, then the process
init reads its configuration file, `/etc/inittab` and executes several scripts that perform final
system initialization.

## Building an Embedded Linux System

Building an embedded Linux system requires you to:

1. Select a `cross toolchain`. The toolchain is the starting point for the development process, as
it is used to build all subsequent software packages. The toolchain consists of the following parts:
Assembler, Compiler, Linker, Debugger, Runtime Libraries adn Utilities. A cross compiler is a compiler
capable of creating executable code for a platform other than the one on which the compiler is running.

2. Select the different packages that will run on the target (Bootloader, Kernel and Root filesystem).

3. Configure and build these packages.

4. Deploy them on the device.

There are several different ways to build an embedded Linux system:

1. `Manually (creating your own scripts)`: this option gives you total control, but it is also tedious
and harder to reproduce builds on other machines. It also requires a good understanding of the software
component installation process.

2. `Using complete distributions` (e.g., Ubuntu/Debian): easy to get started, but harder to customize.
A Linux distribution is a preselected kernel version and a root filesystem with a preselected set of libraries,
utilities and applications.

3. `Using Build frameworks` (e.g., Buildroot, Yocto): This option allows you to customize and reproduce
builds easily. This is becoming the most popular option in the embedded Linux space. A Build framework
typically consists of scripts and configuration meta-data that control the build process. The Build framework
typically downloads, configures, compiles and installs all required components of the system taking version
conflicts and dependencies into account. It allows for example to create a customized root filesystem. The
Build framework output is a complete image including toolchain, bootloader, kernel and root filesystem.

