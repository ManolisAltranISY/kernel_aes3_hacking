#include <linux/version.h>
#include <linux/module.h>
#if defined(MODVERSIONS)
#include <linux/modversions.h>
#endif

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <asm/uaccess.h>
#include <linux/init.h>
//#include <linux/wrapper.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EMMANOUIL NIKOLAKAKIS");
MODULE_DESCRIPTION("Do-nothing test driver");
MODULE_VERSION("0.1");

#define LEN (64*1024)

/* device open */
int mmapdrv_open(struct inode *inode, struct file *file);
/* device close */
int mmapdrv_release(struct inode *inode, struct file *file);
/* device mmap */
int mmapdrv_mmap(struct file *file, struct vm_area_struct *vma);

/* the ordinary device operations */
static struct file_operations mmapdrv_fops =
{
  mmap:    mmapdrv_mmap,
  open:    mmapdrv_open,
  release: mmapdrv_release,
};

/* pointer to page aligned area */
static int *kmalloc_area = NULL;
/* pointer to unaligned area */
static int *kmalloc_ptr = NULL;

/* major number of device */
static int major;

static int __init hello_init(void){

   int i;
   unsigned long virt_addr;

   if ((major=register_chrdev(0, "mmapdrv", &mmapdrv_fops))<0) {
     printk("mmapdrv: unable to register character device\n");
     return (-EIO);
   }

   printk(KERN_INFO "Hello, world. This is Manolis!\n");

   /* get a memory area with kmalloc and aligned it to a page. This area will be physically contigous */
   kmalloc_ptr=kmalloc(LEN+2*PAGE_SIZE, GFP_KERNEL);
   kmalloc_area=(int *)(((unsigned long)kmalloc_ptr + PAGE_SIZE -1) & PAGE_MASK);

   for (virt_addr=(unsigned long)kmalloc_area; virt_addr<(unsigned long)kmalloc_area+LEN; virt_addr+=PAGE_SIZE) {
	    /* reserve all pages to make them remapable */
	    mem_map_reserve(virt_to_page(virt_addr));
   }

   for (i=0; i<(LEN/sizeof(int)); i+=2) {
   /* initialise with some dummy values to compare later */
     kmalloc_area[i]=(0xdead<<16) +i;
     kmalloc_area[i+1]=(0xbeef<<16) + i;
   }

   printk("kmalloc_area at 0x%p (phys 0x%lx)\n", kmalloc_area,
   virt_to_phys((void *)virt_to_kseg(kmalloc_area)));

   return 0;
}

static void __exit hello_exit(void){

  unsigned long virt_addr;

  for(virt_addr=(unsigned long)kmalloc_area; virt_addr<(unsigned long)kmalloc_area+LEN; virt_addr+=PAGE_SIZE) {
                mem_map_unreserve(virt_to_page(virt_addr));
  }

  mem_map_unreserve(virt_to_page(virt_to_kseg((void *)virt_addr)));

  if (kmalloc_ptr) {
    kfree(kmalloc_ptr);
  }
  unregister_chrdev(major, "mmapdrv");
  printk(KERN_INFO "Goodbye, world. Manolis out!\n");
}

/* device open method */
int mmapdrv_open(struct inode *inode, struct file *file)
{
        MOD_INC_USE_COUNT;
        return(0);
}

/* device close method */
int mmapdrv_release(struct inode *inode, struct file *file)
{
        MOD_DEC_USE_COUNT;
        return(0);
}

/* device memory map method */
int mmapdrv_mmap(struct file *file, struct vm_area_struct *vma)
{
  unsigned long offset = vma->vm_pgoff<<PAGE_SHIFT;

  unsigned long size = vma->vm_end - vma->vm_start;

  if (offset & ~PAGE_MASK)
  {
                printk("offset not aligned: %ld\n", offset);
                return -ENXIO;
  }

  if (size>LEN)
  {
                printk("size too big\n");
                return(-ENXIO);
  }
  /* we only support shared mappings. Copy on write mappings are
	   rejected here. A shared mapping that is writeable must have the
	   shared flag set.
	*/
	if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED))
	{
	     printk("writeable mappings must be shared, rejecting\n");
	     return(-EINVAL);
	}

	/* we do not want to have this area swapped out, lock it */
	vma->vm_flags |= VM_LOCKED;

  if (offset == LEN)
  {
    /* enter pages into mapping of application */
    if (remap_page_range(vma->vm_start, virt_to_phys((void*)((unsigned long)kmalloc_area)), size, PAGE_SHARED)) {
      printk("remap page range failed\n");
      return -ENXIO;
    } else {
      printk("offset out of range\n");
      return -ENXIO;
    }
    return(0);
}

module_init(hello_init);
module_exit(hello_exit);
