
/*
 * This application uses functionality found in the driver:
 *
 * chapter_4/lab-3/helloworld_misc_char_driver.c
 *
 * This driver provides an implementation of some of the
 * file_operations functions (open, close, ioctl).
 *
 * Before executing this program, make sure to install the module:
 *
 * 	insmod helloworld_misc_char_driver.ko
 *
 * Note:
 * 	my_misc_dev represents the character device name.
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {
	char *char_dev_name = "/dev/my_misc_dev";
	int my_char_device = open(char_dev_name, O_RDONLY);

	printf("Example of how to use %s driver. Uses the Miscellaneous Framework\n");
	printf("open() returns %d\n", my_char_device);

	if (my_char_device < 0) {
		perror("Failed to open the character device: /dev/my_misc_dev");
	} else {
		ioctl(my_char_device, 123, 89); // cmd = 123, arg = 89
		close(my_char_device);
		printf("Check the kernel output with `dmesg`\n");
	}

	return 0;
}


