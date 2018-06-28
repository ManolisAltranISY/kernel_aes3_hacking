#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>

#define  DEVICE_NAME "testchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "test"        ///< The device class -- this is a character device driver

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EMMANOUIL NIKOLAKAKIS");
MODULE_DESCRIPTION("Do-nothing test driver");
MODULE_VERSION("0.1");

// Necessary variables for this example

static int		majorNumber;			/// Stores the device number, determined automatically
static char		kernel_msg[256] = {0};		/// Memory for the string that is passed from userspace
static short		size_of_kernel_msg;		/// Used to remember the size of the string stored
static int		numberOpens = 0;		/// Counts how many times the device was opened
static struct class* 	testcharClass = NULL;		/// The device-driver class struct pointer
static struct device*	testcharDevice = NULL;		/// The device-driver device struct pointer

static char *aes3_buffer;

// The prototype functions for the driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

static int __init hello_init(void){

   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "TestChar failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "TestChar: registered correctly with major number %d\n", majorNumber);

   // Register the device class
   testcharClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(testcharClass)){              
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(testcharClass);        
   }
 
   // Register the device driver
   testcharDevice = device_create(testcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(testcharDevice)){               
      class_destroy(testcharClass);          
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(testcharDevice);
   }
   printk(KERN_INFO "TestChar: device class created correctly\n"); // 
   
   aes3_buffer = kmalloc(256, GFP_KERNEL); //keep in mind kmalloc goes along with kfree

   return 0;
}

static void __exit hello_exit(void){
    device_destroy(testcharClass, MKDEV(majorNumber, 0));     
    class_unregister(testcharClass);                          
    class_destroy(testcharClass);         
    unregister_chrdev(majorNumber, DEVICE_NAME);             
    printk(KERN_INFO "Goodbye, world. Manolis out!\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
    numberOpens++;
    printk(KERN_INFO "TestChar: Device has been opened %d time(s)\n", numberOpens);
    return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, kernel_msg, size_of_kernel_msg);
 
   if (error_count==0){     
      printk(KERN_INFO "TestChar: Sent %d characters to the user\n", size_of_kernel_msg);
      return (size_of_kernel_msg=0);  // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "TestChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address kernel_msg (i.e. -14)
   }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
	
   sprintf(kernel_msg, "%s(%zu letters and I'm writting some extra info)", buffer, len);   // appending received string with its length
   sprintf(kernel_msg, "%s(%zu letters and I'm writting some extra info)", buffer, len);   // appending received string with its length
   size_of_kernel_msg = strlen(kernel_msg);                 
   printk(KERN_INFO "TestChar: Received %zu characters from the user\n", len);
   return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "TestChar: Device successfully closed\n");
   return 0;
}

module_init(hello_init);
module_exit(hello_exit);

