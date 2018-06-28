/**
 * @file   userspace_hello.c
 * @author Emmanouil Nikolakakis
 * @date   20 June 2018
 * @version 0.1
 * @brief  A Linux user space program that communicates with the test driver hello world.
 */

 #include <stdio.h>
 #include <unistd.h>
 #include <sys/mman.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>

#define LEN (64*1024)

/* this is a test program that opens the mmap_drv. It reads out values of the kmalloc() and vmalloc()
   allocated areas and checks for correctness. You need a device special file to access the driver.
   The device special file is called 'node' and searched in the current directory.
   To create it load the driver 'insmod mmap_mod.o' find the major number assigned to the driver
   'grep mmapdrv /proc/devices' and create the special file (assuming major number 254)
   'mknod node c 254 0'
*/

int main(void)
{

  int fd;
  unsigned int *kadr;
  if ((fd=open("node", O_RDWR))<0)
  {
    perror("open problem");
    return -1;
  }

  kadr = mmap(0, LEN, PROT_READ, MAP_SHARED, fd, LEN);

  if (kadr == MAP_FAILED)
  {
    perror("mmap problem");
    return -1;
  }

  if ((kadr[0]!=0xdead0000) || (kadr[1]!=0xbeef0000)
      || (kadr[LEN/sizeof(int)-2]!=(0xdead0000+LEN/sizeof(int)-2))
      || (kadr[LEN/sizeof(int)-1]!=(0xbeef0000+LEN/sizeof(int)-2)))
  {
    printf("0x%x 0x%x\n", kadr[0], kadr[1]);
    printf("0x%x 0x%x\n", kadr[LEN/sizeof(int)-2], kadr[LEN/sizeof(int)-1]);
  }

  close(fd);
  return(0);
}
