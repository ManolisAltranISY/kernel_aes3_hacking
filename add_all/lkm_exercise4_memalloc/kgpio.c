#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h> //kmalloc & kfree stuff

#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/wrapper.h>

#define TEST_LENGTH () //the example has 64*1024 so I have high expectations
#define  DEVICE_NAME "testchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "test"        ///< The device class -- this is a character device driver


MODULE_LICENSE("GPL");
MODULE_AUTHOR("EMMANOUIL NIKOLAKAKIS");
MODULE_DESCRIPTION("Interrupt Driven GPIO");
MODULE_VERSION("0.1");

// Necessary variables for this example

static int		majorNumber;			/// Stores the device number, determined automatically
static char		kernel_msg[256] = {0};		
static short		size_of_kernel_msg;		
static int		numberOpens = 0;		/// Counts how many times the device was opened
static struct class* 	testcharClass = NULL;		/// The device-driver class struct pointer
static struct device*	testcharDevice = NULL;		/// The device-driver device struct pointer

static unsigned int gpioTest = 10;   /// randomly chose gpio 10 for testing, but it passes the gpio_is_valid condition below so it must be ok
static unsigned int irqNumber;        /// we use this number to link interrupt to gpio pin
static unsigned int numberChanges = 0;  ///< For information, store the number of button presses

/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t  gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

static int *aes3_buffer;

static int result = 0;

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
	
	// gpio initialization	
	
	result = 0;

	if (!gpio_is_valid(gpioTest)){
      printk(KERN_INFO "GPIO_TEST: invalid test GPIO\n");
      return -ENODEV;
   }
   
   gpio_request(gpioTest, "sysfs");
   gpio_direction_input(gpioTest);
   gpio_set_debounce(gpioTest, 200);      // delay of 200ms, uses 200ms to read stabilizes input, should not include in final AES3 code
   gpio_export(gpioTest, false);          // Causes gpio10 to appear in /sys/class/gpio   

	printk(KERN_INFO "GPIO_TEST: The button state is currently: %d\n", gpio_get_value(gpioTest));  //testing functionality, although pin is floating
	
	irqNumber = gpio_to_irq(gpioTest);
   printk(KERN_INFO "GPIO_TEST: The button is mapped to IRQ: %d\n", irqNumber);
   
   result = request_irq(irqNumber, (irq_handler_t) gpio_irq_handler, IRQF_TRIGGER_RISING, "test_gpio_handler", NULL);  //we'll see what we can do to trigger on both rising and falling edges...
 
   printk(KERN_INFO "GPIO_TEST: The interrupt request result is: %d\n", result);
   // return result; I dunno how exactly we'll use this...
   return 0;
   
}

static void __exit hello_exit(void){	

	// Uninitializing the GPIO input pin
	printk(KERN_INFO "GPIO_TEST: The button state is currently: %d\n", gpio_get_value(gpioTest));
   free_irq(irqNumber, NULL);               // Free the IRQ number, no *dev_id required in this case
   gpio_unexport(gpioTest);               // Unexport the Button GPIO
   gpio_free(gpioTest);                   // Free the Button GPIO
   
    // Destroying the device-driver driver and class
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
   sprintf(kernel_msg, "%s(%zu letters and I'm writting some extra info)",  buffer, len);   // appending received string with its length
   size_of_kernel_msg = strlen(kernel_msg);                 
   printk(KERN_INFO "TestChar: Received %zu characters from the user\n", len);
   return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "TestChar: Device successfully closed\n");
   return 0;
}


static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
   printk(KERN_INFO "GPIO_TEST: Interrupt! (button state is %d)\n", gpio_get_value(gpioTest));
   numberChanges++;                         // This is uninitialized atm...deal with it soon
   return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

module_init(hello_init);
module_exit(hello_exit);

