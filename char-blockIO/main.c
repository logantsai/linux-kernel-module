#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/poll.h>

#define MEM_CLEAR 1

#define GLOBALMEM_SIZE  0x1000  /* 4k */

struct hello_dev {
	struct cdev cdev;
	struct mutex mutex;
	/* Use this to block read/write system call */
	unsigned int current_len;
	u8  mem[GLOBALMEM_SIZE];
	wait_queue_head_t r_wait;
	wait_queue_head_t w_wait;
};

#define CDEV_NR 1

static int dev_major = 0;
static int dev_minor;
static struct hello_dev *hello_devp;
static struct class *hello_class;

static loff_t dev_llseek(struct file *filp, loff_t offset, int orig)
{
	loff_t rc = 0;
	switch (orig) {
	case SEEK_SET:    /* seek relative to beginning of file */
		if (offset < 0) {
			rc = -EINVAL;
			break;
		}
		if ((unsigned int)offset > GLOBALMEM_SIZE) {
			rc = -EINVAL;
			break;
		}
		filp->f_pos = (unsigned int)offset;
		rc = filp->f_pos;
		break;
	case SEEK_HOLE:     /* seek relative to current file position */
		if ((filp->f_pos + offset) > GLOBALMEM_SIZE) {
			rc = -EINVAL;
			break;
		}
		if ((filp->f_pos + offset) < 0) {
			rc = -EINVAL;
			break;
		}
		filp->f_pos += offset;
		rc = filp->f_pos;
		break;
	default:
		rc =  -EINVAL;
		break;
	}
	return rc;
}


ssize_t dev_read(struct file *filp, char __user *buf, size_t r_size, loff_t *f_pos)
{
	int rc;
	struct hello_dev *dev = filp->private_data;

	/*create a instant that named "wait" and it's a element of wait queue,
	  bind it to current process.
	*/
	DECLARE_WAITQUEUE(wait, current);

	mutex_lock(&dev->mutex);

	add_wait_queue(&dev->r_wait, &wait);


	/* For read system call, it need to block process by
	   checking current_len equal to zero. */
	while (dev->current_len == 0) {
		if (filp->f_flags & O_NONBLOCK) {
			rc = -EAGAIN;
			goto out;
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		mutex_unlock(&dev->mutex);
		schedule(); /* schedule this process */

		/* If process wakeup and have signal to deal with,
		   return -ERESTARTSYS to user mode process, process will call this systemcall again
		   as it receive -ERESTARTSYS */
		if (signal_pending(current)) {
			rc = -ERESTARTSYS;
			goto out2;
		}
		mutex_lock(&dev->mutex);
	}

	if (r_size > dev->current_len)
		r_size = dev->current_len;

	if (copy_to_user(buf, dev->mem, r_size)) {
		rc = -EFAULT;
		goto out;
	} else {
		memcpy(dev->mem, dev->mem + r_size, dev->current_len - r_size);
		dev->current_len -= r_size;
		printk(KERN_INFO "read %d bytes(s),current_len:%d\n", r_size, dev->current_len);

		/* buf have space to write, wakeup those process that want to write. */
		wake_up_interruptible(&dev->w_wait);
		rc = r_size;
	}

out:
	mutex_unlock(&dev->mutex);
out2:
	remove_wait_queue(&dev->r_wait, &wait);
	set_current_state(TASK_RUNNING);
	return rc;
}

ssize_t dev_write(struct file *filp, const char __user *buf, size_t w_size,
		  loff_t *f_pos)
{
	int rc;
	struct hello_dev *dev = filp->private_data;

	DECLARE_WAITQUEUE(wait, current);

	mutex_lock(&dev->mutex);

	add_wait_queue(&dev->w_wait, &wait);

	while (dev->current_len == GLOBALMEM_SIZE) {
		if (filp->f_flags & O_NONBLOCK) {
			rc = -EAGAIN;
			goto out;
		}

		__set_current_state(TASK_INTERRUPTIBLE);
		mutex_unlock(&dev->mutex);

		schedule();

		if (signal_pending(current)) {
			rc = -ERESTARTSYS;
			goto out2;
		}
	}

	if (w_size > GLOBALMEM_SIZE - dev->current_len)
		w_size = GLOBALMEM_SIZE - dev->current_len;

	if (copy_from_user(dev->mem + dev->current_len, buf, w_size)) {
		rc = -EFAULT;
		goto out;
	} else {
		dev->current_len += w_size;
		printk(KERN_INFO "written %d bytes(s),current_len:%d\n",
			w_size, dev->current_len);

		/* buf have data to read, wakeup those process that want to read. */
		wake_up_interruptible(&dev->r_wait);
		rc = w_size;
	}

out:
	mutex_unlock(&dev->mutex);
out2:
	remove_wait_queue(&dev->w_wait, &wait);
	set_current_state(TASK_RUNNING);
	return rc;
}

static long dev_ioctl(
		struct file *filp,
		unsigned int cmd,
		unsigned long arg
		)
{
	struct hello_dev *devp = filp->private_data;
	switch (cmd) {
		case MEM_CLEAR:
		{
			memset(devp->mem, 0, GLOBALMEM_SIZE);
			printk(KERN_INFO "hello is set to zero\n");
			break;
		}
		default:
		{
			return  -EINVAL;
		}
	}
	return 0;
}

int dev_open(struct inode *inode, struct file *filp)
{
	struct hello_dev *devp = container_of(inode->i_cdev, struct hello_dev, cdev);
	filp->private_data = devp;
	printk("hello%d %s():\n", MINOR(hello_devp->cdev.dev), __FUNCTION__);
	return 0;
}

int dev_release(struct inode *inode, struct file *filp)
{
	printk("%s():\n", __FUNCTION__);
	return 0;
}

static const struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.read = dev_read,
	.write = dev_write,
	.unlocked_ioctl = dev_ioctl,
	.open = dev_open,
	.release = dev_release,
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

	hello_devp = kmalloc(CDEV_NR * sizeof(struct hello_dev), GFP_KERNEL);

	for (i = 0; i < CDEV_NR; ++i) {
		device_create(hello_class, NULL,
						MKDEV(dev_major, i), NULL, "hello%d", i);
		cdev_init(&(hello_devp[i].cdev), &dev_fops);
		hello_devp[i].cdev.owner = THIS_MODULE;
		hello_devp[i].current_len = 0;
		memset(hello_devp[i].mem, 0, GLOBALMEM_SIZE);
		rc = cdev_add(&(hello_devp[i].cdev), MKDEV(dev_major, i), 1);
		if (rc < 0) {
			printk("add chr dev failed\n");
			goto failed;
		}
	}

	mutex_init(&hello_devp->mutex);
	init_waitqueue_head(&hello_devp->r_wait);
	init_waitqueue_head(&hello_devp->w_wait);

	printk(KERN_DEBUG "hello world!\n");
	return 0;

failed:
	unregister_chrdev_region(hello_devnr, CDEV_NR);
	return 0;
}

void helloexit(void)
{
	dev_t hello_devnr;
	int i;

	printk(KERN_DEBUG "goodbye world!!\n");
	for (i = 0; i < CDEV_NR; ++i) {
		device_destroy(hello_class, MKDEV(dev_major, i));
		cdev_del(&(hello_devp[i].cdev));
	}
	class_destroy(hello_class);
	unregister_chrdev_region(hello_devnr, CDEV_NR);
}

module_init(helloinit);
module_exit(helloexit);

MODULE_LICENSE("GPL");
