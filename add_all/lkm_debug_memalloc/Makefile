obj-m+= updated_kmalloc.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} modules
	$(CC) updated_ukmalloc.c -o updated_ukmalloc
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} clean
	rm updated_ukmalloc
