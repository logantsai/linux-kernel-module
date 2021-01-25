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

#define CDEV_NR 1
#define DEV_BUFSIZE 1024

static int dev_major = 0;
static int dev_minor;
static struct cdev *hello_dev[CDEV_NR];
static struct class *hello_class;

static int send_uevent(struct device *dev)
{
	char event_string[26];
	char *envp[] = {event_string, NULL};

	strncpy(event_string, "IG2_POWER_EVENT=TEST", 26);
	pr_err("kobject: '%s' (%p): %s\n", kobject_name(&(dev->kobj)),
		&(dev->kobj), __func__);
	kobject_uevent_env( &(dev->kobj), KOBJ_CHANGE, envp );
	return 0;
}

struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.open = NULL,
	.release = NULL,
	.read = NULL,
	.write = NULL,
};

int helloinit(void)
{
	int rc = 0, i = 0;
    struct device *this_dev;
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
		this_dev = device_create(hello_class, NULL,
						MKDEV(dev_major, i), NULL, "hello%d", i);
		hello_dev[i] = cdev_alloc();
		cdev_init(hello_dev[i], &dev_fops);
		hello_dev[i]->owner = THIS_MODULE;
		rc = cdev_add(hello_dev[i], MKDEV(dev_major, i), 1);
		if (rc < 0) {
			printk("add chr dev failed\n");
			goto failed;
		}
	}

	printk(KERN_DEBUG "hello world!\n");

    send_uevent(this_dev);

    return 0;

failed:
	for (i  = 0; i < CDEV_NR; ++i) {
		if (hello_dev[i]) {
			kfree(hello_dev[i]);
			hello_dev[i] = NULL;
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
		cdev_del(hello_dev[i]);
	}
	class_destroy(hello_class);
	unregister_chrdev_region(hello_devnr, CDEV_NR);
}

module_init(helloinit);
module_exit(helloexit);

MODULE_LICENSE("GPL");
