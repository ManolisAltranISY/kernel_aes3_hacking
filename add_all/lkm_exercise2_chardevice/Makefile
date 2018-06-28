obj-m+= kernelspace_hello.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} modules
	$(CC) userspace_hello.c -o us_hello
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} clean
	rm us_hello