
# Note: in order to execute this Makefile, the linaro cross compiler
# has to be in the path. Another option is to set the CROSS_COMPILE
# environment variable and then we refer to this variable in this
# file as $(CROSS_COMPILE). 

obj-m += helloworld.o helloworld_with_parameters.o helloworld_with_timing.o


KERNEL_DIR ?= $(HOME)/Development/linux-driver-dev/linux-kernel

all:
	make -C $(KERNEL_DIR) \
		ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- \
		SUBDIRS=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) \
		ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- \
		SUBDIRS=$(PWD) clean

# This option is used when you have connected the Raspberry Pi to
# the host using an ethernet connection. SCP stands for secure copy.
deploy:
	scp *.ko root@10.0.0.10:
	