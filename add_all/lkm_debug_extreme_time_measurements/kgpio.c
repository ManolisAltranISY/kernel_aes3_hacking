#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EMMANOUIL NIKOLAKAKIS");
MODULE_DESCRIPTION("Interrupt Driven GPIO");
MODULE_VERSION("0.1");

static unsigned int gpioTest = 10;   /// randomly chose gpio 10 for testing, but it passes the gpio_is_valid condition below so it must be ok
module_param(gpioTest, uint, S_IRUGO);

static char   message[64] = {0};           ///< Memory for the string that is passed from userspace
static unsigned int count = 0;
static unsigned int count2 = 0;

static unsigned int gpioOutput = 8;   /// this one shall create some output to use as an input to pin 10 for testing
static unsigned int irqNumber;        /// we use this number to link interrupt to gpio pin
int result = 0;
static struct timespec ts_last, ts_current, ts_diff;

static irq_handler_t  gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

static int __init hello_init(void){

   getnstimeofday(&ts_last); //last time to be the current time
   ts_diff = timespec_sub(ts_last, ts_last); //initial time difference to 0

   gpio_direction_input(gpioTest);
   gpio_direction_output(gpioOutput, true);

	 printk(KERN_INFO "GPIO_TEST: The button state is currently: %d\n", gpio_get_value(gpioTest));  //testing functionality, although pin is floating
	 irqNumber = gpio_to_irq(gpioTest);
   result = request_irq(irqNumber, (irq_handler_t) gpio_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "test_gpio_handler", NULL);  //we'll see what we can do to trigger on both rising and falling edges...

   getnstimeofday(&ts_current); //gets current time as ts_current
   ts_last = ts_current;

   return 0;
}

static void __exit hello_exit(void){

	 printk(KERN_INFO "GPIO_TEST: The button state is currently: %d\n", gpio_get_value(gpioTest));
   free_irq(irqNumber, NULL);               // Free the IRQ number, no *dev_id required in this case
   gpio_unexport(gpioTest);               // Unexport the Button GPIO
   gpio_free(gpioTest);                   // Free the Button GPIO

   gpio_unexport(gpioOutput);               // Unexport the Button GPIO
   gpio_free(gpioOutput);                   // Free the Button GPIO

   printk(KERN_INFO "Goodbye, world. Manolis out!\n");
}

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
   message[count] = gpio_get_value(gpioTest);
   count++;
   if (count > 15) {
     getnstimeofday(&ts_current);
     ts_diff = timespec_sub(ts_current, ts_last); //determine the difference between last two state changes
     ts_last = ts_current; //current time is the last time as ts_last
     printk(KERN_INFO "GPIO_TEST: Time required to get the latest 64 edges is %lu, %lu)\n", ts_diff.tv_sec, ts_diff.tv_nsec);
     for (count2 = 0; count2 < 16; count2++){
       printk(KERN_INFO "GPIO_TEST: Input number %d (button state is %d)\n", count2, message[count2]);
     }
     getnstimeofday(&ts_current); //gets current time as ts_current
     ts_last = ts_current;
     count = 0;
   }
   return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

module_init(hello_init);
module_exit(hello_exit);
