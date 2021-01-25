#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/workqueue.h>



struct delayed_work hello_handler;

static void hello_status(struct work_struct *work)
{
    printk("hello_status delay work run\n");
}

int helloinit(void)
{
    INIT_DELAYED_WORK(&hello_handler, hello_status);
    schedule_delayed_work(&hello_handler, 2000);

	printk(KERN_DEBUG "hello world!\n");
	return 0;

}

void helloexit(void)
{
    cancel_delayed_work_sync(&hello_handler);
}

module_init(helloinit);
module_exit(helloexit);

MODULE_LICENSE("GPL");
