obj-m+= k_novirt.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} modules
	$(CC) u_mem.c -o u_mem
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} clean
	rm u_mem
