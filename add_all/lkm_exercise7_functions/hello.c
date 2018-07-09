#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EMMANOUIL NIKOLAKAKIS");
MODULE_DESCRIPTION("Do-nothing test driver");
MODULE_VERSION("0.1");

//static struct task_struct *task;            /// The pointer to the thread task
// static int * test = 290194;
static int function_example(void);


static int function_example(){
    printk(KERN_INFO "I'm currently within a function");
    return 1;
}

static int __init hello_init(void){
    printk(KERN_INFO "Hello, world. This is Manolis!\n");
    function_example();
    return 0;
}

static void __exit hello_exit(void){
    printk(KERN_INFO "Goodbye, world. Manolis out!\n");
}

module_init(hello_init);
module_exit(hello_exit);
