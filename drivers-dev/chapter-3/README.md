
# Chapter 3 - The Simplest Drivers

A key concept in the design of an embedded Linux system is the
separation of user applications from the underlying hardware. User space
applications are not allowed to access peripheral registers, storage media
or even RAM memory directly. Instead, the harware is accessed via kernel
drivers, and RAM memory is managed by the memory management unit (MMU) with
applications operating on virtual addresses.

Device drivers can be kernel modules or statically built into the kernel image.
The default kernel builds most drivers into the kernel statically, so they are started
automatically. A kernel module is not necessarily a device driver, it is an extension of
the kernel. The kernel modules are loaded into virtual memory of the kernel. Building a
device driver as a module makes the development easier since it can be loaded, tested, and
unloaded without rebooting the kernel.

Kernel statically compiled modules are installed into `/lib/module/<kernel_version>/`
on the root filesystem.

A list of utilities to install and unistall modules is below:

- insmod module.ko
- rmmod module
- lsmod
- modinfo module_name
- modprobe module [ module parameters ]
