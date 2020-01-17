
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
