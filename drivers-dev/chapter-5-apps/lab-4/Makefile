
# The crosscompiler has to be in the path.

CC = arm-linux-gnueabihf-gcc

all: app_for_uio_platform_driver

app : app_for_uio_platform_driver.c
	$(CC) -o $@ $^

clean :
	rm app_for_uio_platform_driver

deploy : app_for_uio_platform_driver
	scp $^ root@10.0.0.10:

