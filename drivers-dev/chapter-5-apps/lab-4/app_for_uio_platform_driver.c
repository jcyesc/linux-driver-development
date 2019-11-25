
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>


#define BUFFER_LENGHT 128

#define GPIO_27			27
#define GPIO_22			22
#define GPIO_26			26

#define GPFSEL2_offset 	 0x08
#define GPSET0_offset    0x1C
#define GPCLR0_offset 	 0x28

/* to set and clear each individual LED */
#define GPIO_27_INDEX 	1 << (GPIO_27 % 32)
#define GPIO_22_INDEX 	1 << (GPIO_22 % 32)
#define GPIO_26_INDEX 	1 << (GPIO_26 % 32)

/* select the output function */
#define GPIO_27_FUNC	1 << ((GPIO_27 % 10) * 3)
#define GPIO_22_FUNC 	1 << ((GPIO_22 % 10) * 3)
#define GPIO_26_FUNC 	1 << ((GPIO_26 % 10) * 3)

#define FSEL_27_MASK 	0b111 << ((GPIO_27 % 10) * 3) /* red since bit 21 (FSEL27) */
#define FSEL_22_MASK 	0b111 << ((GPIO_22 % 10) * 3) /* green since bit 6 (FSEL22) */
#define FSEL_26_MASK 	0b111 << ((GPIO_26 % 10) * 3) /* blue since bit 18 (FSEL26) */

#define GPIO_SET_FUNCTION_LEDS (GPIO_27_FUNC | GPIO_22_FUNC | GPIO_26_FUNC)
#define GPIO_MASK_ALL_LEDS 	(FSEL_27_MASK | FSEL_22_MASK | FSEL_26_MASK)
#define GPIO_SET_ALL_LEDS (GPIO_27_INDEX  | GPIO_22_INDEX  | GPIO_26_INDEX)

#define UIO_DEVICE_PATH "/dev/uio0"
#define UIO_MEM_SIZE "/sys/class/uio/uio0/maps/map0/size"

/*
 * The driver uio_platform_driver.c will map the physical memory of the device and
 * then:
 *
 * - The client will open the device /dev/uio0 to get a file descriptor
 * - The client will get the memory size.
 * - The client will create a memory map using mmap().
 * - The client will read and write in that memory in order to control the leds.
 */
int main()
{
	int ret;
	int devuio_fd;
	int mem_fd;
	unsigned int uio_mem_size;
	void *temp;
	int GPFSEL_read;
	int GPFSEL_write;
	void *device_memory_map; /* Device memory mapped by the UIO platform driver */
	char send_string[BUFFER_LENGHT];
	char *led_on = "on";
	char *led_off = "off";
	char *exit_msg = "exit";

	printf("Starting UIO platform driver client example\n");

	/* try to get the file descriptor for the UIO device */
	devuio_fd = open(UIO_DEVICE_PATH, O_RDWR | O_SYNC);
	if (devuio_fd < 0) {
		perror("Failed to open the device");
		exit(EXIT_FAILURE);
	}

	printf("UIO device opened %s \n", UIO_DEVICE_PATH);

	/* Read the size that has to be mapped */
	FILE *mem_size = fopen(UIO_MEM_SIZE, "r");
	if (!mem_size) {
		perror("Failed to open the file that contains the memory size");
		exit(EXIT_FAILURE);
	}
	fscanf(mem_size, "0x%08X", &uio_mem_size);
	fclose(mem_size);

	/* Do the mapping */
	device_memory_map = mmap(NULL, uio_mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, devuio_fd, 0);
	if (device_memory_map == MAP_FAILED) {
		perror("The deviceuio failed to be mapped!!");
		close(devuio_fd);
		exit(EXIT_FAILURE);
	}

	GPFSEL_read = *(int *) (device_memory_map + GPFSEL2_offset);

	GPFSEL_write = (GPFSEL_read & ~GPIO_MASK_ALL_LEDS) |
			(GPIO_SET_FUNCTION_LEDS & GPIO_MASK_ALL_LEDS);

	/* Set direction leds to output */
	*(int *)(device_memory_map + GPFSEL2_offset) = GPFSEL_write;
	/* Clear all the leds, output is low */
	*(int *)(device_memory_map + GPCLR0_offset) = GPIO_SET_ALL_LEDS;

	/* Control the LED */
	do {
		printf("Enter led value: on, off, or exit: \n");
		scanf("%[^\n]%*c", send_string);

		if(strncmp(led_on, send_string, 3) == 0) {
			temp = device_memory_map + GPSET0_offset;
			*(int *)temp = GPIO_27_INDEX;
		} else if(strncmp(led_off, send_string, 2) == 0) {
			temp = device_memory_map + GPCLR0_offset;
			*(int *)temp = GPIO_27_INDEX;
		} else if(strncmp(exit_msg, send_string, 4) == 0) {
			printf("Exit application\n");
		} else {
			printf("Bad value\n");
			return -EINVAL;
		}
	} while (strncmp(send_string, exit_msg, strlen(send_string)));

	ret = munmap(device_memory_map, uio_mem_size);
	if (ret < 0) {
		perror("devuio munmap");
		close(devuio_fd);
		exit(EXIT_FAILURE);
	}

	close(devuio_fd);
	printf("End UIO platform driver client example");
	exit(EXIT_SUCCESS);
}


