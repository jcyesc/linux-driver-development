
# Chapter 9 - Linux DMA in Device Drivers


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
