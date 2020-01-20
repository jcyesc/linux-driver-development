
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#define DMA_BUF_SIZE  (1024*63)

int main(void)
{
	char *virtaddr;
	char phrase[128];
	int my_dev = open("/dev/dma_mmap", O_RDWR);
	if (my_dev < 0) {
		perror("Fail to open device file: /dev/sdma_test.");
	} else {
		printf("Enter phrase :\n");
		scanf("%[^\n]%*c", phrase);

		virtaddr = (char *)mmap(0, DMA_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, my_dev, 0);
		printf("Copying %s to mmap\n", phrase);
		strcpy(virtaddr, phrase);
		printf("Copied %s\n", virtaddr);

		ioctl(my_dev, NULL);
		close(my_dev);
	}

	return 0;
}

