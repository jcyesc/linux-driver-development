
/*
 * This application uses functionality found in the driver:
 *
 * chapter_4/lab-1/helloworld_char_driver.c
 *
 * This driver provides an implementation of some of the
 * file_operations functions (open, close, ioctl).
 *
 * Before executing this program, make sure that the following command
 * has been executed:
 *
 *		cat /proc/devices
 *		mknod /dev/my_char_device c 202 0
 *
 * Note:
 * 	/dev/my_char_device represents the character device name.
 * 	c indicates that is a character device.
 * 	202 represents the major number
 * 	0 represents the minor number
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>


int main(void) {
	int my_char_device = open("/dev/my_char_device", 0 /* O_RDONLY flag */);
	printf("Example of how to use my_char_device's driver\n");

	if(my_char_device < 0) {
		perror("Failed to open the character device: /dev/my_char_device\n");
	} else {
		ioctl(my_char_device, 199, 110);  // cmd = 100, arg = 110
		close(my_char_device);
	}
}


