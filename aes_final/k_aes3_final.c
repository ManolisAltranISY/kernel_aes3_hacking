#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

// Declaring module parameters
MODULE_LICENSE("GPL");
MODULE_AUTHOR("EMMANOUIL NIKOLAKAKIS");
MODULE_DESCRIPTION("Interrupt Driven GPIO");
MODULE_VERSION("0.1");

static char aes3_buffer[8] = (0);

// Declaring peripherals
static unsigned int gpioPCM0 = 1;
static unsigned int gpioPCM1 = 2;
static unsigned int gpioPCM2 = 3;
static unsigned int gpioPCM3 = 4;
static unsigned int gpioPCM4 = 5;
static unsigned int gpioPCM5 = 6;
static unsigned int gpioPCM6 = 7;
static unsigned int gpioPCM7 = 8;
static unsigned int gpioAES3 = 10;
static unsigned int irqNumber;        /// we use this number to link interrupt to gpio pin

// Declaringn functions to be used
static int preamble_detection(unsigned int latest_value);
static int convert_biphase_to_ttl(void);

/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t  gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

static int __init hello_init(void){

	// gpio initialization

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
   gpio_free(gpioPCM0);                   // Free the Button GPIO
   gpio_free(gpioPCM1);                   // Free the Button GPIO
   gpio_free(gpioPCM2);                   // Free the Button GPIO
   gpio_free(gpioPCM3);                   // Free the Button GPIO
   gpio_free(gpioPCM4);                   // Free the Button GPIO
   gpio_free(gpioPCM5);                   // Free the Button GPIO
	gpio_free(gpioPCM6);                   // Free the Button GPIO
	gpio_free(gpioPCM7);                   // Free the Button GPIO
   gpio_free(gpioAES3);                   // Free the Button GPIO
}

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
  aes3_buffer[0] = gpio_get_value(gpioPCM0); 
  aes3_buffer[1] = gpio_get_value(gpioPCM1); 
  aes3_buffer[2] = gpio_get_value(gpioPCM2); 
  aes3_buffer[3] = gpio_get_value(gpioPCM3); 
  aes3_buffer[4] = gpio_get_value(gpioPCM4); 
  aes3_buffer[5] = gpio_get_value(gpioPCM5); 
  aes3_buffer[6] = gpio_get_value(gpioPCM6); 
  aes3_buffer[7] = gpio_get_value(gpioPCM7); 
 )

module_init(hello_init);
module_exit(hello_exit);
