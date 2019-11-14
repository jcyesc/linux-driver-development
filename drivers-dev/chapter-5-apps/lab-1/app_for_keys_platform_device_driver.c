
/*
 * This application uses functionality found in the driver:
 *
 * chapter_5/lab-1/keys_platform_device_driver.c
 *
 * This driver provides an implementation of some of the
 * file_operations functions (open, close, ioctl).
 *
 * Before executing this program, make sure to install the module:
 *
 * 	insmod keys_platform_device_driver.ko
 *
 * Note:
 * 	/dev/keys_dev represents the keys device name.
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {
	char *keys_dev_name = "/dev/keys_dev";
	int keys_device = open(keys_dev_name, O_RDONLY);

	printf("Example that uses a Platform Driver\n");
	printf("open() returns %d\n", keys_device);
	if (keys_dev_name < 0) {
		perror("Failed to open the keys device: /dev/keys_dev\n");
	} else {
		ioctl(keys_device, 150, 111); // cmd = 150, arg = 111
		close(keys_device);
	}

	return 0;
}

