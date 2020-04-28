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
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/version.h>

#define CDEV_NR 1

static int dev_major = 0;
static int dev_minor;

struct hello_dev {
	struct cdev cdev;
	atomic_t counter;   /* total second */
	struct timer_list s_timer; /* The timer this device use.*/
};

static struct hello_dev *hello_devp[CDEV_NR];
static struct class *hello_class;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,14,162)
static void second_timer_handle(unsigned long arg)
{
#else
static void second_timer_handle(struct timer_list *timers)
{
#endif
	mod_timer(&(hello_devp[0]->s_timer), jiffies + 3*HZ);
	atomic_inc(&(hello_devp[0]->counter));

	printk(KERN_NOTICE "current jiffies is %ld\n", jiffies);
}

ssize_t dev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int counter;
	counter = atomic_read(&(hello_devp[0]->counter));
	if(put_user(counter, (int*)buf))
		return -EFAULT;
	else
		return sizeof(unsigned int);
}

int dev_open(struct inode *inode, struct file *filp)
{
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,14,162)
	init_timer(&(hello_devp[0]->s_timer));
	hello_devp[0]->s_timer.function = &second_timer_handle;
#else
    DEFINE_TIMER(s_timer, second_timer_handle);
	hello_devp[0]->s_timer = s_timer;
#endif

	printk("address of timer %p, %p", &hello_devp[0]->s_timer, &s_timer);

	/* Arrive time: this means that trigger timer after 1s. */
	hello_devp[0]->s_timer.expires = jiffies + 3*HZ;

	add_timer(&(hello_devp[0]->s_timer));
	/*clear the coutner */
	atomic_set(&hello_devp[0]->counter, 0);
	return 0;
}

int dev_release(struct inode *inode, struct file *filp)
{
	del_timer(&(hello_devp[0]->s_timer));
	return 0;
}

struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_release,
	.read = dev_read,
};

int helloinit(void)
{
	int rc = 0, i = 0;
	dev_t hello_devnr;

	if (dev_major) {
		hello_devnr = MKDEV(dev_major, dev_minor);
		rc = register_chrdev_region(hello_devnr, CDEV_NR, "hello");
	} else {
		/* LDD3 recommand alloc_chrdev_region to instead of register_chrdev_region,
		   alloc_chrdev_region will allocate major number dynamiclly.
		*/
		rc = alloc_chrdev_region(&hello_devnr, 0, CDEV_NR, "hello");
	}

	if (rc < 0) {
		printk("can't get major\n");
		goto failed;
	}

	dev_major = MAJOR(hello_devnr);
	dev_minor = MINOR(hello_devnr);
	printk("register chrdev(%d,%d)\n", dev_major, dev_minor);
	hello_class = class_create(THIS_MODULE, "hello_class");


	for (i = 0; i < CDEV_NR; ++i) {

		hello_devp[i] = kmalloc(CDEV_NR * sizeof(struct hello_dev), GFP_KERNEL);
		device_create(hello_class, NULL,
						MKDEV(dev_major, i), NULL, "hello%d", i);
		cdev_init(&(hello_devp[i]->cdev), &dev_fops);
		rc = cdev_add(&(hello_devp[i]->cdev), MKDEV(dev_major, i), 1);
		if (rc < 0) {
			printk("add chr dev failed\n");
			goto failed;
		}
	}

	printk(KERN_DEBUG "hello world!\n");
	return 0;

failed:
	for (i  = 0; i < CDEV_NR; ++i) {
		if (hello_devp[i]) {
			kfree(hello_devp[i]);
			hello_devp[i] = NULL;
		}
	}
	return 0;
}

void helloexit(void)
{
	dev_t hello_devnr;
	int i;

	printk(KERN_DEBUG "goodbye world!!\n");
	for (i = 0; i < CDEV_NR; ++i) {
		device_destroy(hello_class, MKDEV(dev_major, i));
		cdev_del(&(hello_devp[i]->cdev));
	}
	class_destroy(hello_class);
	unregister_chrdev_region(hello_devnr, CDEV_NR);
}

module_init(helloinit);
module_exit(helloexit);

MODULE_LICENSE("GPL");
