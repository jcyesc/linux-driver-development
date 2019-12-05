
# Note: in order to execute this Makefile, the linaro cross compiler
# has to be in the path. Another option is to set the CROSS_COMPILE
# environment variable and then we refer to this variable in this
# file as $(CROSS_COMPILE).
# Once that the .ko file is created, we need to copy it to the target (Raspberry pi)
# and install it with the "insmod" command.

obj-m := net_gpio_driver_module.o
net_gpio_driver_module-objs := net_gpio_controller.o net_gpio_driver.o

KERNEL_DIR ?= $(HOME)/Development/linux-driver-dev/linux-kernel

all:
	make -C $(KERNEL_DIR) \
		ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- \
		SUBDIRS=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) \
		ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- \
		SUBDIRS=$(PWD) clean

deploy:
	scp *.ko root@10.0.0.10:

