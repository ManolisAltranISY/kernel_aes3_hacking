obj-m+= kgpio.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} modules
	$(CC) ugpio.c -o ugpio
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} clean
	rm ugpio