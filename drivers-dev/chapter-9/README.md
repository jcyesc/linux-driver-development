
# Chapter 9 - Linux DMA in Device Drivers

Direct Memory Access (DMA) is the hardware mechanism that allows peripheral components to
transfer their I/O data directly to and from main memory without the need to involve the
system processor. Use of this mechanism can greatly increase throughput to and from a device,
because a great deal of computational overhead is eliminated.

The CPU manages DMA operations via a DMA controller unit. While the DMA transfers is in
progresss, the CPU can continue executing code. When the DMA transfer is completed, the
DMA controller will signal the CPU with an interrupt.

Typical scenarios of block memory copy where DMA can be useful are network packet routing
and video streaming. DMA is a particular advantage in situations where the blocks to be
transferred are large or the transfer is a repetitive operation that would consume a large
portion of potentially useful CPU processing time.

## Cache Coherency

On processors with a data cache an unwanted side effect of using DMA is the possibility that
the contents of the cache are no longer coherent with respect to main memory, which can lead
to data corruption problems. Imagine a CPU equipped with a cache and external memory that can
be accessed directly by devices using DMA. When the CPU tries to access data X located in the
main memory, it could happen that the current value has been chached by the procesor, then
subsequent operations on X will update the cached copy of X, but not the external memory version
of X, assuming a write-back cache. If the cache is not flushed to the main memory before the next
time a device (DMA) tries to transfer X, the device will receive a stale value of X. Similarly,
if the cached copy of X is not invalidated before a device (DMA) writes a new value to the main
memory, then the CPU will operate on a stale value of X. Also, when the cache is flushed, the stale
data will be written back to the main memory overwriting the new data stored by the DMA. The end
result is that the data in main memory is not correct.

Some processors include a mechanism called bus snooping or cache snooping; the snooping hardware
notices when an external DMA transfer refers to main memory using an address that matches data in
the cache, and either flushes/invalidates the cache entry so that the DMA transfers the correct
data and the state of the cache entry is updated accordingly. These systems are called `coherent
architectures` providing a hardware to take care of cache coherency related problem. Hardware will
itself maintain coherency between caches and main memory and will ensure that all subsystem (CPU
and DMA) have the same view of the memory.

Fo `non-coherent architectures`, the device driver should explicitly flush or invalidate the data
cache before initiating a transfer or making data buffers available to bus mastering peripherals.
This can also complicate the software and will cause more transfers between cache and main memory,
but it does allow the application to use any arbitrary region of cached memory as a data buffer.


## Linux DMA Engine API

The Linux DMA Engine API specifies an interface to the actual DMA controller hardware functionality
to initialize/clean-up and perform DMA transfers.

The slave DMA API usage consists of following steps:

- Allocate a DMA slave channel
- Set slave and controller specific parameters
- Get a descriptor for transaction
- Submit the transaction
- Issue pending requests and wait for callback notification


## Type of addresses involved in the Linux DMA API

There are several kinds of addresses involved in the Linux DMA API, and it's important to
understand the differences. The kernel normally uses virtual addresses. Any address returned
by `kmalloc()`, `vmalloc()`, and similar interfaces is a virtual address.

The virtual memory system translates virtual addresses to CPU physical addresses, which are
stored as `phys_addr_t` or `resource_site_t`.

The kernel manages device resources like registers as physical addresses. These are the addresses
in `/proc/iomem`. The physical address is not directly useful to a driver; it must use the
`ioremap()` function to map the space and produce a virtual address.

I/O devices use a third kind of address: a `bus address`. If a device performs DMA to read or
write system memory, the addresses used by the device are bus address. In many systems, bus
addresses are identical to CPU physical addresses.

If the device supports DMA, the driver sets up a buffer by using kmalloc() or a similar interface,
which returns a virtual address (X). The virtual memory system maps X to a physical address (Y)
in system RAM. The driver can use virtual address X to access the buffer, but the device itself
cannot because DMA doesn't go through the CPU virtual memory system. This is part of the reason
for the DMA API: the driver can give a virtual address X to an interface like `dma_map_single()`,
which returns the DMA bus address (Z). The driver then tells the device to perform DMA to Z.

The memory accessed by the DMA should be physically contiguos. Any memory allocated by kmalloc()
`(up to 128 KB)` or __get_free_pages() `(up to 8MB)` can be used. What cannot be used is
`vmalloc()` memory allocation (it would have to setup DMA on each individual physical page).


## Types of DMA Mappings

1. `Coherent DMA Mappings`: use uncached memory mapping from kernel space, usually allocated by
using `dma_alloc_coherent()`. The kernel allocates a suitable buffer and sets the mapping for the
driver. Can simultaneously be accessed by the CPU and device, so this has to be in a cache coherent
memory area. Usually allocated for the whole time the module is loaded. Buffers are usually mapped
at driver initialization, ummapped at the end and to do this the hardware should guarantee that the
device and the CPU can access the data in parallel and will see updates made by each other without any
explicit software flushing.

2. `Streaming DMA Mappings`: use cached mappings and clean or invalidate it according to the
operation needed by using `dma_map_single()` and `dma_unmmap_single()`. This is different from
coherent mapping because the mappings deal with addresses that were chosen apriori, which are usually
mapped for one DMA transfer and ummapped right after it.


## DMA Scather/Gather Mappings

Scatter/gather I/O allows the system to perform DMA I/O operations or buffers which are scattered
throughout physical memory. Consider for example the case of a large (multi-page) buffer created
in user space. The application sees a contigous range of virtual address, but the physical pages
behind those address will almost certainly not be adjacent to each other. If that buffer is to be
written to a device in a single I/O operation, one of two things must be done:

1. The data must be copied into a physically contiguous buffer.
2. The device must be able to work with a list of physical address and lenghts, grabbing the right
amount of data from each segment.

Scatter/gather I/O, by eliminating the need to copy data into contigous buffers, can greatly increase
the efficiency of I/O operations while simultaneously getting around the problem generated by the
creation of large, physically-contiguous buffers.


## Lab 1 - DMA memory to memory copy

This driver will perform the following tasks:

- Allocate two buffers: write_buffer and read_buffer
- Create a device (/dev/dma_m2m_misc_dev) that the will be used to write to write_buffer.
- The write() file operation will set up a DMA transaction memory to memory that will
copy the content of write_buffer to read_buffer.
- At the end of the DMA transaction the buffers will be compared.

The files used for this lab are:

- chapter-9/lab-1-dma-m2m/dma_m2m.c
- chapter-9/lab-1-dma-m2m/Makefile (builds the .ko file)
- linux-kernel/arch/arm/boot/dts/bcm2710-rpi-3-b.dts

>Note: Remember to build the device tree source (DTS) and install the modules.

```shell
sudo insmod dma_m2m.ko
ls /dev/dma_m2m_misc_dev
tree /sys/class/misc/dma_m2m_misc_dev
sudo chmod 666 /dev/dma_m2m_misc_dev
echo one-two-three > /dev/dma_m2m_misc_dev
sudo rmmod dma_m2m
```

## Lab 2 - DMA memory and memory map driver

> Note: This driver doesn't work properly in the Raspberry Pi. After an app
> call ioctl(), the driver doesn't respond anymore and the DMA transaction is
> not executed.

The goal of the driver is to create a mmap and copy the data from the mmap
to another part in the memory using a DMA transaction.

- We will use the dma_ioctl() kernel callback insteam of dma_write() to manage
the DMA transaction.

- The dma_mmap() callback function is added to the driver to do the mapping of
the kernel buffer.

- The virtual address of the process will be returned to the user space by using
mmap() system call. Any text can be written from the user application to the returned
virtual memory buffer. After that, the ioctl() system call manages the DMA transaction
sending the written text from the dma_src buffer to the dma_dst buffer without any
CPU intervention.

The files used for this lab are:

- chapter-9/lab-2-dma-mmap/app/dma_mmap_app.c
- chapter-9/lab-2-dma-mmap/app/Makefile
- chapter-9/lab-2-dma-mmap/driver/dma_mmap_driver.c
- chapter-9/lab-2-dma-mmap/driver/Makefile (builds the .ko file)
- linux-kernel/arch/arm/boot/dts/bcm2710-rpi-3-b.dts

>Note: Remember to build the device tree source (DTS) and install the modules.


```shell
sudo insmod dma_mmap_driver.ko
sudo chmod 777 /dev/dma_mmap 
./dma_mmap_app
sudo rmmod dma_mmap_driver
```
