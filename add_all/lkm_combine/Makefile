obj-m+= k_combined.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} modules
	$(CC) u_combined.c -o aes3_conv
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=${PWD} clean
	rm u_combined
