obj-m+= kernelspace_memset2.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} modules
	$(CC) userspace_memset.c -o userspace_memset
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} clean
	rm userspace_memset
