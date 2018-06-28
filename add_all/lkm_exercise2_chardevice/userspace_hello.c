/**
 * @file   userspace_hello.c
 * @author Emmanouil Nikolakakis
 * @date   20 June 2018
 * @version 0.1
 * @brief  A Linux user space program that communicates with the test driver hello world.
 */

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
 
#define BUFFER_LENGTH 256              
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM
 
int main(){

   int ret, fd;
   char test_string[BUFFER_LENGTH] = "do not end";
   char *exercise_string;
   exercise_string = (char *) malloc(BUFFER_LENGTH);
   strcpy (exercise_string, "do not end");
   printf("Starting device test code example...\n");
   fd = open("/dev/testchar", O_RDWR);             // Open the device with read/write access

   if (fd < 0){
      perror("Failed to open the device...");
      return errno;
   }

	while (1) {
		
  		printf("Type in a short string to send to the kernel module, enter 'end' if termination is your desire...\n");
   	scanf("%[^\n]%*c", test_string);
   	printf("Writing message to the device [%s].\n", test_string);
   	ret = write(fd, test_string, strlen(test_string)); // Send the string to the LKM

   	if (ret < 0){
      	perror("Failed to write the message to the device.");
     	 return errno;
   	}
 
   	printf("Press ENTER to read back from the device...\n");
   	getchar();
 
   	printf("Reading from the device...\n");
   	ret = read(fd, receive, BUFFER_LENGTH);        // reading the response from the lkm

   	if (ret < 0){
        perror("Failed to read the message from the device.");
    	  return errno;
   	}
   	
   	printf("The received message is: [%s]\n", receive);
   	
	}
	
   printf("The program has ended.\n");
   return 0;
}