
# The crosscompiler has to be in the path.

CC = arm-linux-gnueabihf-gcc

all: app_for_helloworld_misc_char_driver

app : app_for_helloworld_misc_char_driver.c
	$(CC) -o $@ $^

clean :
	rm app_for_helloworld_misc_char_driver

deploy : app_for_helloworld_misc_char_driver
	scp $^ root@10.0.0.10:

