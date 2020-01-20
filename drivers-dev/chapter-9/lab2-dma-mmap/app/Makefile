
# The crosscompiler has to be in the path.

CC = arm-linux-gnueabihf-gcc

all: dma_mmap_app

app : dma_mmap_app.c
	$(CC) -o $@ $^
	
clean :
	rm dma_mmap_app
	
deploy : dma_mmap_app
	scp dma_mmap_app pi@192.168.2.2:/home/pi/my_modules


