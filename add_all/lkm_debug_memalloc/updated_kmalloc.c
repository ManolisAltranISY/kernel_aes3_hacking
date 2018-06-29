#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

#define MAX_SIZE (PAGE_SIZE * 2)   /* max size mmaped to userspace */
#define DEVICE_NAME "kbuffer_aes3"
#define  CLASS_NAME "kbuffer"

static struct class*  class;
static struct device*  device;
static int major;
static char *shared_memory = NULL;
static int *kmalloc_area = NULL;

int count = 0;

static DEFINE_MUTEX(kbuffer_aes3_mutex);

static int kbuffer_aes3_release(struct inode *inodep, struct file *filep)
{
    mutex_unlock(&kbuffer_aes3_mutex);
    pr_info("kbuffer_aes3: Device successfully closed\n");
    return 0;
}

static int kbuffer_aes3_open(struct inode *inodep, struct file *filep)
{
    int ret = 0;

    if(!mutex_trylock(&kbuffer_aes3_mutex)) {
        pr_alert("kbuffer_aes3: device busy!\n");
        ret = -EBUSY;
    }

    pr_info("kbuffer_aes3: Device opened\n");
    return ret;
}

/*  mmap handler to map kernel space to user space */
static int kbuffer_aes3_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int ret = 0;
    struct page *page = NULL;
    unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);

    if (size > MAX_SIZE) {
        ret = -EINVAL;
    }

    page = virt_to_page((unsigned long)shared_memory + (vma->vm_pgoff << PAGE_SHIFT));
    ret = remap_pfn_range(vma, vma->vm_start, page_to_pfn(page), size, vma->vm_page_prot);
    return ret;
}

static ssize_t kbuffer_aes3_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    int ret = 0;
    if (len > MAX_SIZE) {
        pr_info("read overflow!\n");
        ret = -EFAULT;
    }
    if (copy_to_user(buffer, shared_memory, len) == 0) {
        pr_info("kbuffer_aes3: copy %zu char to the user\n", len);
        ret = len;
    } else {
        ret =  -EFAULT;
    }
    return ret;
}

static const struct file_operations kbuffer_aes3_fops = {
    .open = kbuffer_aes3_open,
    .read = kbuffer_aes3_read,
    .release = kbuffer_aes3_release,
    .mmap = kbuffer_aes3_mmap,
    /*.unlocked_ioctl = kbuffer_aes3_ioctl,*/
    .owner = THIS_MODULE,
};

static int __init kbuffer_aes3_init(void)
{
    int ret = 0;
    major = register_chrdev(0, DEVICE_NAME, &kbuffer_aes3_fops);

    if (major < 0) {
        pr_info("kbuffer_aes3: fail to register major number!");
        return -1;
    }

    class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(class)){
        unregister_chrdev(major, DEVICE_NAME);
        pr_info("kbuffer_aes3: failed to register device class");
        return -1;
    }

    device = device_create(class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(device)) {
        class_destroy(class);
        unregister_chrdev(major, DEVICE_NAME);
        return -1;
    }
    /* init this mmap area */
    shared_memory = kmalloc(MAX_SIZE, GFP_KERNEL);
    kmalloc_area=(int *)(((unsigned long)shared_memory + PAGE_SIZE -1) & PAGE_MASK);
    // for (count = 0; count<(64/sizeof(int)); count+=2) {
    // /* initialise with some dummy values to compare later */
    //   kmalloc_area[count]=(0xbeef<<16) +count;
    //   kmalloc_area[count+1]=(0xbeef<<16) + count;
    // }

    kmalloc_area[0]=(0x40);
    kmalloc_area[1]=(0x80);


    if (shared_memory == NULL) {
        ret = -ENOMEM;
    }
    sprintf(shared_memory, "xyz\n");
    mutex_init(&kbuffer_aes3_mutex);
    return ret;
}

static void __exit kbuffer_aes3_exit(void)
{
    mutex_destroy(&kbuffer_aes3_mutex);
    device_destroy(class, MKDEV(major, 0));
    class_unregister(class);
    class_destroy(class);
    unregister_chrdev(major, DEVICE_NAME);
    //kfree(shared_memory);

    pr_info("kbuffer_aes3: unregistered!");
}

module_init(kbuffer_aes3_init);
module_exit(kbuffer_aes3_exit);
