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


#define CDEV_NR 2
#define DEV_BUFSIZE 1024

static int dev_major = 0;
static int dev_minor;
static struct cdev *hello_dev[CDEV_NR];
static struct class *hello_class;

ssize_t dev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	char data[] = "mark";
	ssize_t ret = 0;

	printk("%s():\n", __FUNCTION__);

	if (*f_pos >= sizeof(data)) {
		goto out;
	}

	if (count > sizeof(data)) {
		count = sizeof(data);
	}

	if (copy_to_user(buf, data, count) < 0) {
		ret = -EFAULT;
		goto out;
	}
	*f_pos += count;
	ret = count;

out:
	return ret;

}

ssize_t dev_write(struct file *filp, const char __user *buf, size_t count,
		  loff_t *f_pos)
{
	char *data;
	ssize_t ret = 0;

	printk("%s():\n", __FUNCTION__);

	data = kzalloc(sizeof(char) * DEV_BUFSIZE, GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}

	if (count > DEV_BUFSIZE) {
		count = DEV_BUFSIZE;
	}

	if (copy_from_user(data, buf, count) < 0) {
		ret = -EFAULT;
		goto out;
	}
	printk("%s(): %s\n", __FUNCTION__, data);
	*f_pos += count;
	ret = count;
out:
	kfree(data);
	return ret;

}

int dev_open(struct inode *inode, struct file *filp)
{
	printk("%s():\n", __FUNCTION__);
	return 0;
}

int dev_release(struct inode *inode, struct file *filp)
{
	printk("%s():\n", __FUNCTION__);
	return 0;
}

struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_release,
	.read = dev_read,
	.write = dev_write,
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
		device_create(hello_class, NULL,
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
