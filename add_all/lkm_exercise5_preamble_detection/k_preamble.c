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

// couldn't be bothered with timer based interrupts

static unsigned int gpioTest = 10; // mocks an input
module_param(gpioTest, uint, S_IRUGO);
static unsigned int gpioOutput = 8;

static char   preamble_pattern[8] = {1, 1, 0, 0, 1, 1, 0, 0};
static char   mock_pattern[32] = {1,0,1,1,0,1,1,0,0,1,0,0,1,0,0,1,1,1,0,0,1,1,0,0,1,1,1,1,0,0,0,0};
static char   latest_eight_digits[8] = {0};
static int    current_mock = 0;
static int    preamble_detected = 0;
static int    test_complete = 0;
static unsigned int count = 0;

static unsigned int irqNumber;
int result = 0;

static irq_handler_t  gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

static int __init hello_init(void){

   if (!gpio_is_valid(gpioOutput)){
      printk(KERN_INFO "PREAMBLE_TEST: invalid output GPIO\n");
      return -ENODEV;
   }

   if (!gpio_is_valid(gpioTest)){
      printk(KERN_INFO "PREAMBLE_TEST: invalid test GPIO\n");
      return -ENODEV;
   }

   gpio_direction_input(gpioTest);
   gpio_direction_output(gpioOutput, mock_pattern[current_mock]);       // value is the state
   current_mock++;
	 printk(KERN_INFO "PREAMBLE_TEST: The button state is currently: %d\n", gpio_get_value(gpioTest));  //testing functionality, although pin is floating
	 irqNumber = gpio_to_irq(gpioTest);
   result = request_irq(irqNumber, (irq_handler_t) gpio_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "test_gpio_handler", NULL);  //we'll see what we can do to trigger on both rising and falling edges...

   return 0;
}

static void __exit hello_exit(void){

	 printk(KERN_INFO "PREAMBLE_TEST: The button state is currently: %d\n", gpio_get_value(gpioTest));
   free_irq(irqNumber, NULL);               // Free the IRQ number, no *dev_id required in this case
   gpio_free(gpioTest);                   // Free the test GPIO
   gpio_free(gpioOutput);                   // Free the output GPIO
   printk(KERN_INFO "Goodbye, world. Manolis out!\n");
}

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
   gpio_set_value(gpioOutput, mock_pattern[current_mock]);
   current_mock++;
   preamble_detected = 1;
   for (count = 0; count < 8; count++){
     latest_eight_digits [8-count] = latest_eight_digits [8 - count - 1];
   }
   latest_eight_digits[0] = gpio_get_value(gpioTest);
   for (count = 0; count < 8 || preamble_detected == 1 || test_complete == 0; count++){
     if (latest_eight_digits[count] != preamble_pattern[count]){
       preamble_detected = 0;
     }
   }
   if (preamble_detected == 1){
     test_complete = 1;
     printk(KERN_INFO "PREAMBLE_TEST: We have successfully detected a preamble pattern.\n");
   }
   return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

module_init(hello_init);
module_exit(hello_exit);
