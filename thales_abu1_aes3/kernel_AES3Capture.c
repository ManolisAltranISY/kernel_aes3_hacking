/*
 * @file   AES3Capture.c
 * @author Emmanouil Nikolakakis
 * @date   30/05/2018
 * @brief  A kernel module used to capture AES3 data at a 6MHz frequency from
 * a GPIO and interrupt combination (timer to capture 0s, edges for 1s)
 * The input data will need to be converted from biphase mark code to a simple
 * high/low state format before parsing to the user space
 * The sysfs entry appears at /sys/abu_aes3/aes3
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>     // Supports the kernel driver model
#include <linux/fs.h>         // Linux file system support
#include <linux/uaccess.h>    // Required for the copy to user function
#include <linux/gpio.h>       // Required for the GPIO functions
#include <linux/kobject.h>    // Using kobjects for the sysfs bindings
#include <linux/kthread.h>    // Using kthreads for the flashing functionality
#include <linux/delay.h>      // Using this header for the msleep() function
#include <linux/mutex.h>      // Used to lock & release shared resources
#include <linux/interrupt.h>  // Used to generate irq on edges & timer rollbacks

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emmanouil Nikolakakis");
MODULE_DESCRIPTION("An AES3 capture module");
MODULE_VERSION("0.1");

static unsigned int gpioAES3 = 10;           ///< Default GPIO for the LED is 10
static unsigned int irqNumber; //< Used to share the IRQ number within this file

/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t  AES3gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

module_param(gpioAES3, uint, S_IRUGO);       ///< Param desc. S_IRUGO can be read/not changed
MODULE_PARM_DESC(gpioAES3, " GPIO AES3 number (default=49)");     ///< parameter description

enum modes {OFF, ON};              ///< The available LED modes -- static not useful here
static enum modes mode = ON;             ///< Default mode is ON

/** @brief A callback function to display the conversion mode
 *  @param kobj represents a kernel object device that appears in the sysfs filesystem
 *  @param attr the pointer to the kobj_attribute struct
 *  @param buf the buffer to which to write the number of presses
 *  @return return the number of characters of the mode string successfully displayed
 */
static ssize_t mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
   switch(mode){
      case OFF:   return sprintf(buf, "off\n");       // Display the state -- simplistic approach
      case ON:    return sprintf(buf, "on\n");
      default:    return sprintf(buf, "LKM Error\n"); // Cannot get here
   }
}

/** @brief A callback function to store the conversion mode using the enum above */
static ssize_t mode_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
   // the count-1 is important as otherwise the \n is used in the comparison
   if (strncmp(buf,"on",count-1)==0) { mode = ON; }   // strncmp() compare with fixed number chars
   else if (strncmp(buf,"off",count-1)==0) { mode = OFF; }
   return count;
}

/** Use these helper macros to define the name and access levels of the kobj_attributes
 *  The kobj_attribute has an attribute attr (name and mode), show and store function pointers
 *  The period variable is associated with the blinkPeriod variable and it is to be exposed
 *  with mode 0666 using the period_show and period_store functions above
 */
static struct kobj_attribute mode_attr = __ATTR(mode, 0666, mode_show, mode_store);

/** The ebb_attrs[] is an array of attributes that is used to create the attribute group below.
 *  The attr property of the kobj_attribute is used to extract the attribute struct
 */
static struct attribute *ebb_attrs[] = {
   &mode_attr.attr,                         // Is the LED on or off?
   NULL,
};

/** The attribute group uses the attribute array and a name, which is exposed on sysfs -- in this
 *  case it is gpio49, which is automatically defined in the ebbLED_init() function below
 *  using the custom kernel parameter that can be passed when the module is loaded.
 */
static struct attribute_group attr_group = {
   .name  = ledName,                        // The name is generated in ebbLED_init()
   .attrs = ebb_attrs,                      // The attributes array defined just above
};

static struct kobject *aes3_kobj;            /// The pointer to the kobject
static struct task_struct *task;            /// The pointer to the thread task

/** @brief The LED Flasher main kthread loop
 *
 *  @param arg A void pointer used in order to pass data to the thread
 *  @return returns 0 if successful
 */
static int flash(void *arg){
   printk(KERN_INFO "AES3: Thread has started running \n");
   while(!kthread_should_stop()){           // Returns true when kthread_stop() is called
      set_current_state(TASK_RUNNING);
      if (mode==FLASH) ledOn = !ledOn;      // Invert the LED state
      else if (mode==ON) ledOn = true;
      else ledOn = false;
      set_current_state(TASK_INTERRUPTIBLE);
      msleep(blinkPeriod/2);                // millisecond sleep for half of the period
   }
   printk(KERN_INFO "AES3: Thread has run to completion \n");
   return 0;
}

// Manolis: the interrupt handler copy pasted as it appears in the tutorial
static irq_handler_t aes3_gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs) {
   // the actions that the interrupt should perform...basically read and store the GPIO state
   }

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point. In this example this
 *  function sets up the GPIOs and the IRQ
 *  @return returns 0 if successful
 */
static int __init aes3conv_init(void){
   int result = 0;

   printk(KERN_INFO "AES3: Initializing the AES3 Capture LKM\n");
   sprintf(ledName, "led%d", gpioAES3);      // Create the gpio115 name for /sys/ebb/led49

   aes3_kobj = kobject_create_and_add("aes3", kernel_kobj->parent); // kernel_kobj points to /sys/kernel
   if(!aes3_kobj){
      printk(KERN_ALERT "AES3: failed to create kobject\n");
      return -ENOMEM;
   }
   // add the attributes to /sys/ebb/ -- for example, /sys/ebb/led49/ledOn
   result = sysfs_create_group(ebb_kobj, &attr_group);
   if(result) {
      printk(KERN_ALERT "AES3: failed to create sysfs group\n");
      kobject_put(aes3_kobj);                // clean up -- remove the kobject sysfs entry
      return result;
   }
   ledOn = true;
   gpio_request(gpioAES3, "sysfs");          // gpioLED is 49 by default, request it
   gpio_direction_input(gpioAES3);   // Set the gpio to be in output mode and turn on
   gpio_export(gpioAES3, false);  // causes gpio49 to appear in /sys/class/gpio
                                 // the second argument prevents the direction from being changed

   // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
   irqNumber = gpio_to_irq(gpioAES3);
   printk(KERN_INFO "GPIO_TEST: The AES3 GPIO pin is mapped to IRQ: %d\n", irqNumber);

   // This next call requests an interrupt line
   result = request_irq(irqNumber, (irq_handler_t) aes3_gpio_irq_handler, IRQF_TRIGGER_RISING, "AES3_gpio_handler", NULL);

   task = kthread_run(flash, NULL, "LED_flash_thread");  // Start the LED flashing thread
   if(IS_ERR(task)){                                     // Kthread name is LED_flash_thread
      printk(KERN_ALERT "AES3: failed to create the task\n");
      return PTR_ERR(task);
   }
   return result;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit aes3conv_exit(void){
   kthread_stop(task);                      // Stop the LED flashing thread
   kobject_put(ebb_kobj);                   // clean up -- remove the kobject sysfs entry
   gpio_unexport(gpioAES3);                  // Unexport the Button GPIO
   gpio_free(gpioAES3);                      // Free the LED GPIO
   printk(KERN_INFO "AES3: Conversion is complete\n");
}

/// These next calls are  mandatory -- they identify the initialization function
/// and the cleanup function (as above).
module_init(aes3conv_init);
module_exit(aes3conv_exit);
