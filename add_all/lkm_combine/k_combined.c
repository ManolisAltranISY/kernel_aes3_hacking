#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/mm.h>
#ifdef MODVERSIONS
#  include <linux/modversions.h>
#endif
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EMMANOUIL NIKOLAKAKIS");
MODULE_DESCRIPTION("Interrupt Driven GPIO");
MODULE_VERSION("0.1");

//preambles in the patter for reference
#define preamble_X_1 11100010 // 0xE2
#define preamble_X_2 00011101 // 0x1D
#define preamble_Y_1 11100100 // 0xE3
#define preamble_Y_2 00011011 // 0x1B
#define preamble_Z_1 11101000 // 0xE8
#define preamble_Z_2 00010111 // 0x17

//macros foe each part of a frame
#define LEFT_CHANNEL 0
#define SUBFRAME_EMPTY_1 1
#define RIGHT_CHANNEL 2
#define SUBFRAME_EMPTY_2 3

//macros related to biphase mark conversion
#define L_TIME  1
#define S_TIME  0
#define START_OF_CYCLE 0
#define MID_CYCLE 1

/* character device structure for mmap */
static dev_t mmap_dev;
static struct cdev mmap_cdev;
static struct class *mmap_class;
static struct device *mmap_device;
/* methods of the character device */
static int mmap_mmap(struct file *filp, struct vm_area_struct *vma);
/* the file operations, i.e. all character device methods */
static struct file_operations mmap_fops = {
        .mmap = mmap_mmap,
};
// length of the two memory areas
#define NPAGES 16
// pointer to the kmalloc'd area, rounded up to a page boundary
static int *kmalloc_area;
// original pointer for kmalloc'd area as returned by kmalloc
static void *kmalloc_ptr;
static int *kmalloc_ptri;

static unsigned int gpio_AES3Input = 10;   /// randomly chose gpio 14 for testing, but it passes the gpio_is_valid condition below so it must be ok
module_param(gpio_AES3Input, uint, S_IRUGO);

static char   aes3_buffer[12288] = {0};           ///< Memory for the buffer parsed in userspace
static unsigned int number_of_frames = 0;
static unsigned int buffer_count = 0;
static unsigned int count = 0;
static unsigned int current_stage_of_subframe = 0;
static unsigned int previous_state = 0;

static char   preamble_pattern[8] = {1, 1, 1, 0, 1, 0, 0, 0};
static char   latest_eight_digits[8] = {0};
static int    preamble_detected = 0;
static int    temp_value_for_preamble = 0;

int i = 0;
int ret = 0;
long length = 0;

static unsigned int irqNumber;        /// we use this number to link interrupt to gpio pin
int result = 0;
static struct timespec ts_last, ts_current, ts_diff;

static irq_handler_t  gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

static int preamble_detection(void);

// helper function, mmap's the kmalloc'd area which is physically contiguous
int mmap_kmem(struct file *filp, struct vm_area_struct *vma)
{
        length = vma->vm_end - vma->vm_start;
        /* check length - do not allow larger mappings than the number of
           pages allocated */
        if (length > NPAGES * PAGE_SIZE)
                return -EIO;
        /* map the whole physically contiguous area in one piece */
        if ((ret = remap_pfn_range(vma, vma->vm_start, virt_to_phys((void *)kmalloc_area) >> PAGE_SHIFT, length, vma->vm_page_prot)) < 0) {
                return ret;
        }
		pr_err("mmap_kmem Success\n");
		for(i=0;i<10;i++)
			pr_err("ptr %p : %x\n",(kmalloc_ptri + i) ,*(int *)(kmalloc_ptri + i));
        return 0;
}

/* character device mmap method */
static int mmap_mmap(struct file *filp, struct vm_area_struct *vma)
{
        /* at offset NPAGES we map the kmalloc'd area */
        if (vma->vm_pgoff == NPAGES) {
				pr_err("kmalloc mmap\n");
                return mmap_kmem(filp, vma);
        }
        return -EIO;
}


static int preamble_detection(){
    printk(KERN_INFO "Calibration: Detecting audio block preamble...");
    if (ts_diff.tv_nsec == S_TIME && previous_state == START_OF_CYCLE)
    {
        temp_value_for_preamble = 1;
        previous_state = MID_CYCLE;
    }
    else if (ts_diff.tv_nsec == S_TIME && previous_state == MID_CYCLE)
    {
        previous_state = START_OF_CYCLE;
    }
    else if (ts_diff.tv_nsec == L_TIME)
    {
        temp_value_for_preamble = 0;
    }
    for (count = 0; count < 8; count++){
      latest_eight_digits [8-count] = latest_eight_digits [8 - count - 1];
    }
    latest_eight_digits[0] = temp_value_for_preamble;
    preamble_detected = 1;
    for (count = 0; count < 8 || preamble_detected == 1 || 0; count++){
        if (latest_eight_digits[count] != preamble_pattern[count]){
         preamble_detected = 0;
        }
    }
    printk(KERN_INFO "Calibration: We have successfully detected a preamble pattern, conversion started.\n");
    return preamble_detected;
}

static int __init hello_init(void){
    gpio_request(gpio_AES3Input, "sysfs");
    gpio_direction_input(gpio_AES3Input);
    gpio_export(gpio_AES3Input, false);
    printk(KERN_INFO "AES3_TEST: Hello, the button state is currently: %d\n", gpio_get_value(gpio_AES3Input));  //testing functionality, although pin is floating
    irqNumber = gpio_to_irq(gpio_AES3Input);
    result = request_irq(irqNumber, (irq_handler_t) gpio_irq_handler, IRQF_TRIGGER_FALLING, "test_gpio_handler", NULL);  //we'll see what we can do to trigger on both rising and falling edges...
    getnstimeofday(&ts_current); //gets current time as ts_current
    ts_last = ts_current;
    getnstimeofday(&ts_last); //last time to be the current time
    ts_diff = timespec_sub(ts_last, ts_last); //initial time difference to 0

    int ret = 0;
    int i;
    /* allocate a memory area with kmalloc. Will be rounded up to a page boundary */
    if ((kmalloc_ptr = kmalloc((NPAGES + 2) * PAGE_SIZE, GFP_KERNEL)) == NULL) {
        ret = -ENOMEM;
    }
		kmalloc_ptri = kmalloc_ptr;
		memset(kmalloc_ptr, 0x55, (NPAGES + 2) * PAGE_SIZE);
    for(i=0;i<10;i++){
        pr_err("ptr %p : %x\n",(kmalloc_ptri + i) ,*(int *)(kmalloc_ptri + i));
        /* round it up to the page boundary */
        kmalloc_area = (int *)((((unsigned long)kmalloc_ptr) + PAGE_SIZE - 1) & PAGE_MASK);
        /* get the major number of the character device */
        if ((ret = alloc_chrdev_region(&mmap_dev, 0, 1, "mmap")) < 0) {
            printk(KERN_ERR "could not allocate major number for mmap\n");
        }
        /* create device node by udev API */
        mmap_class = class_create(THIS_MODULE, "mmap");
        if (IS_ERR(mmap_class)) {
            return PTR_ERR(mmap_class);
        }
        mmap_device = device_create(mmap_class, NULL, mmap_dev, NULL, "mmap");
        pr_info("mmap module init, major number = %d, device name = %s \n", MAJOR(mmap_dev), dev_name(mmap_device));
        /* initialize the device structure and register the device with the kernel */
        cdev_init(&mmap_cdev, &mmap_fops);
        if ((ret = cdev_add(&mmap_cdev, mmap_dev, 1)) < 0) {
            printk(KERN_ERR "could not allocate chrdev for mmap\n");
        }
        /* mark the pages reserved */
        pr_err("kmalloc_area phyaddress = %lx\n", virt_to_phys(kmalloc_area));
        pr_err("kmalloc_ptr phyaddress = %lx\n", virt_to_phys(kmalloc_ptr));
        pr_err("kmalloc_ptr address = %lx\n", kmalloc_ptr);
        memset(kmalloc_ptr, 0xEE, (NPAGES + 2) * PAGE_SIZE);
        for(i=0;i<10;i++){
            pr_err("ptr %p : %x\n",(kmalloc_ptri + i) ,*(int *)(kmalloc_ptri + i));
            /* store a pattern in the memory - the test application will check for it */
            for (i = 0; i < (NPAGES * PAGE_SIZE / sizeof(int)); i += 2) {
                kmalloc_area[i] = (0xdeed << 16) + i;
                kmalloc_area[i + 1] = (0xbaaf << 16) + i;
            }
      }
  }
    return 0;
}

static void __exit hello_exit(void){
    free_irq(irqNumber, NULL);               // Free the IRQ number, no *dev_id required in this case
    gpio_unexport(gpio_AES3Input);
    gpio_free(gpio_AES3Input);                   // Free the Button GPIO
    printk(KERN_INFO "AES3_TEST: Goodbye, world. Manolis out!\n");

    /* remove the character deivce */
    cdev_del(&mmap_cdev);
    unregister_chrdev_region(mmap_dev, 1);
    /* unreserve the pages */
    for (i = 0; i < NPAGES * PAGE_SIZE; i+= PAGE_SIZE) {
        SetPageReserved(virt_to_page(((unsigned long)kmalloc_area) + i));
    }
    kfree(kmalloc_ptr);
}

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
    printk(KERN_INFO "WELCOME TO THE INTERRUPT MANOLI\n");
    getnstimeofday(&ts_current);
    ts_diff = timespec_sub(ts_current, ts_last); //determine the difference between last two state changes
    ts_last = ts_current; //current time is the last time as ts_last
    if (preamble_detected == 0)
    {
        preamble_detected = preamble_detection();
    }
    else
    {
        if (current_stage_of_subframe == LEFT_CHANNEL || current_stage_of_subframe ==  RIGHT_CHANNEL)
        {
            if (ts_diff.tv_nsec == S_TIME && previous_state == START_OF_CYCLE)
            {
                aes3_buffer[buffer_count] = 1;
                buffer_count++;
                previous_state = MID_CYCLE;
            }
            else if (ts_diff.tv_nsec == S_TIME && previous_state == MID_CYCLE)
            {
                previous_state = START_OF_CYCLE;
            }
            else if (ts_diff.tv_nsec == L_TIME)
            {
                aes3_buffer[count] = 0;
                buffer_count++;
            }
            count++;
            if (count>63)
            {
                if (current_stage_of_subframe == LEFT_CHANNEL)
                {
                    current_stage_of_subframe = SUBFRAME_EMPTY_1;
                }
                else
                {
                    current_stage_of_subframe = SUBFRAME_EMPTY_2;
                }
                count = 0;
            }
        }
        else if (current_stage_of_subframe == SUBFRAME_EMPTY_1)
        {
            count++;
                if (count>191) //used to pass the 192 empty bits (3 Empty)
                {
                    count = 0;
                    current_stage_of_subframe = RIGHT_CHANNEL;
                }
        }
        else if (current_stage_of_subframe == SUBFRAME_EMPTY_2)
        {
            count++;
                if (count>447) //used to pass the 447 empty bits (7 Empty)
                {
                    count = 0;
                    current_stage_of_subframe = LEFT_CHANNEL;
                    number_of_frames++;
                    if (number_of_frames > 192) //due to empty frames, in terms of AES3, it's 192*6 = 1152 frames
                    {
                        number_of_frames = 0;
                        // do something that will say, hey, here, take my buffer
                    }
                }
          }
    }
    return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

module_init(hello_init);
module_exit(hello_exit);
