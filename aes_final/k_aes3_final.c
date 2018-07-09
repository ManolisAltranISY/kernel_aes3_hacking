#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h> //kmalloc & kfree stuff

// Device & class
#define  DEVICE_NAME "aes3_conversion"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "aes3"        ///< The device class -- this is a character device driver

// Biphase to TTL conversion
#define L_TIME  1
#define S_TIME  0
#define START_OF_CYCLE 0
#define MID_CYCLE 1

// Ignoring unpopulated bits
#define LEFT_CHANNEL 0
#define SUBFRAME_EMPTY_1 1
#define RIGHT_CHANNEL 2
#define SUBFRAME_EMPTY_2 3

// Declaring module parameters
MODULE_LICENSE("GPL");
MODULE_AUTHOR("EMMANOUIL NIKOLAKAKIS");
MODULE_DESCRIPTION("Interrupt Driven GPIO");
MODULE_VERSION("0.1");

// Necessary variables for this example
static int		majorNumber;			/// Stores the device number, determined automatically
static struct class* 	aes3Class = NULL;		/// The device-driver class struct pointer
static struct device*	aes3Device = NULL;		/// The device-driver device struct pointer

// Preamble detection variable declaration
static char   preamble_pattern[8] = {1, 1, 1, 0, 0, 0, 1, 0};
static char   latest_eight_digits[8] = {0};
static int    current_mock = 0;
static int    preamble_detected = 0;
static unsigned int count = 0;
static int    latest_value = 0;
static int    i = 0;

// Declaring peripherals
static unsigned int gpioAES3 = 10;
static unsigned int irqNumber;        /// we use this number to link interrupt to gpio pin

// Declaringn functions to be used
static int preamble_detection(unsigned int latest_value);
static int convert_biphase_to_ttl(void);

/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t  gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

static struct file_operations fops =
{
  // whatever
};

static int result = 0;

static int __init hello_init(void){

	 // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);

   if (majorNumber<0){
      printk(KERN_ALERT "AES3_conversion failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "AES3_conversion: registered correctly with major number %d\n", majorNumber);

   // Register the device class
   aes3Class = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(aes3Class)){
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(aes3Class);
   }

   // Register the device driver
   aes3Device = device_create(aes3Class, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(aes3Device)){
      class_destroy(aes3Class);
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(aes3Device);
   }
   printk(KERN_INFO "AES3_conversion: device class created correctly\n"); //

	// gpio initialization

	result = 0;

	if (!gpio_is_valid(gpioAES3)){
      printk(KERN_INFO "AES3_conversion: invalid test GPIO\n");
      return -ENODEV;
   }

   gpio_request(gpioAES3, "sysfs");
   gpio_direction_input(gpioAES3);

	 printk(KERN_INFO "AES3_conversion: The button state is currently: %d\n", gpio_get_value(gpioAES3));  //testing functionality, although pin is floating

	 irqNumber = gpio_to_irq(gpioAES3);
   printk(KERN_INFO "AES3_conversion: The button is mapped to IRQ: %d\n", irqNumber);

   result = request_irq(irqNumber, (irq_handler_t) gpio_irq_handler, IRQF_TRIGGER_RISING, "test_gpio_handler", NULL);  //we'll see what we can do to trigger on both rising and falling edges...

   printk(KERN_INFO "AES3_conversion: The interrupt request result is: %d\n", result);
   // return result; I dunno how exactly we'll use this...
   return 0;

}

static void __exit hello_exit(void){

	// Uninitializing the GPIO input pin
	printk(KERN_INFO "AES3_conversion: The button state is currently: %d\n", gpio_get_value(gpioAES3));
   free_irq(irqNumber, NULL);               // Free the IRQ number, no *dev_id required in this case
   gpio_free(gpioAES3);                   // Free the Button GPIO

    // Destroying the device-driver driver and class
    device_destroy(aes3Class, MKDEV(majorNumber, 0));
    class_unregister(aes3Class);
    class_destroy(aes3Class);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "Goodbye, world. Manolis out!\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
    return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   return 0;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   return 0;
}

static int dev_release(struct inode *inodep, struct file *filep){
   return 0;
}

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
  latest_value = gpio_get_value(gpioAES3);

  count++;
  if(count > 64) {
    convert_biphase_to_ttl();
  }
  preamble_detection(latest_value);
  return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

static int convert_biphase_to_ttl(){
  static int previous_state = START_OF_CYCLE;
  static int time_measured[10] = {L_TIME, S_TIME, S_TIME, L_TIME, S_TIME, S_TIME, L_TIME, L_TIME, S_TIME, S_TIME}; //mock buffer of how input will be converted before parsed
  static int previous_time = L_TIME;
  int value = 0;
  for(i = 0; i<=10; i++)
  {
      if (time_measured[i] == S_TIME && previous_state == START_OF_CYCLE)
      {
        value = 1;
        previous_state = MID_CYCLE;
        printk(KERN_INFO "The value is %d\n", value);
      }
      else if (time_measured[i] == S_TIME && previous_state == MID_CYCLE)
      {
        previous_state = START_OF_CYCLE;
      }
      else if (time_measured[i] == L_TIME)
      {
        value = 0;
        printk(KERN_INFO "The value is %d\n", value);
      }
  }
  return 1;
}

static int preamble_detection(unsigned int latest_value){
  for (count = 0; count < 8; count++){
    latest_eight_digits [8-count] = latest_eight_digits [8 - count - 1];
  }
  latest_eight_digits[0] = latest_value;
  preamble_detected = 1;
  for (count = 0; count < 8; count++){
    if (latest_eight_digits[count] != preamble_pattern[count]){
      preamble_detected = 0;
    }
  }
  if (preamble_detected == 1){
    return 1;
  } else {
    return 0;
  }
}

module_init(hello_init);
module_exit(hello_exit);
