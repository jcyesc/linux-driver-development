
/*
 * This application uses functionality found in the driver:
 *
 * chapter_4/lab-2/helloworld_class_driver.c
 *
 * This driver provides an implementation of some of the
 * file_operations functions (open, close, ioctl).
 *
 * Before executing this program, make sure to install the module:
 *
 * 	insmod helloworld_class_driver.ko
 *
 * Note:
 * 	my_char_class_dev represents the character device name.
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {
	char *char_dev_name = "/dev/my_char_class_dev";
	int my_char_device = open(char_dev_name, O_RDONLY);

	printf("Example of how to use my_char_class_dev's driver\n");

	if (my_char_device < 0) {
		perror("Failed to open the character device: /dev/my_char_class_dev\n");
	} else {
		ioctl(my_char_device, 250, 44); // cmd = 250, arg = 44
		close(my_char_device);
	}

	return 0;
}

