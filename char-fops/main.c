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

#define MEM_CLEAR 1

#define GLOBALMEM_SIZE  0x1000  /* 4k */

struct hello_dev {
    struct cdev cdev;
    u8  mem[GLOBALMEM_SIZE];
};

#define CDEV_NR 2
#define DEV_BUFSIZE 1024

static int dev_major = 0;
static int dev_minor;
static struct hello_dev *hello_devp;
//static struct cdev *hello_dev[CDEV_NR];
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


ssize_t dev_read(struct file *filp, char __user *buf, size_t size, loff_t *f_pos)
{
    unsigned long p =  *f_pos;
    unsigned int count = size;
    int rc = 0;
    struct hello_dev *dev = filp->private_data;

    printk(KERN_INFO "read() size_t: %d, loff_t: %lld \n", size, *f_pos);

    /* The position we want to read is out of range. */
    if (p >= GLOBALMEM_SIZE)
        return count ? -ENXIO : 0;

    /* The remaind space can't satisfy the requirement size */
    if (count > GLOBALMEM_SIZE - p)
        count = GLOBALMEM_SIZE - p;

    /* copy from mem[p] to mem[p+count] */
    if (copy_to_user(buf, (void*)(dev->mem + p), count)) {
        rc = -EFAULT;
    } else {
        *f_pos += count;
        rc = count;
    }

    printk(KERN_INFO "read %d bytes(s) from %d\n", count, p);
    return rc;
}

ssize_t dev_write(struct file *filp, const char __user *buf, size_t size,
          loff_t *f_pos)
{
    unsigned long p =  *f_pos;
    unsigned int count = size;
    int rc = 0;
    struct hello_dev *dev = filp->private_data;

    printk(KERN_INFO "write() size_t: %d, loff_t: %lld \n", size, *f_pos);

    if (p >= GLOBALMEM_SIZE)
        return count ? -ENXIO : 0;

    if (count > GLOBALMEM_SIZE - p)
        count = GLOBALMEM_SIZE - p;

    if (copy_from_user(dev->mem + p, buf, count))
        rc =  -EFAULT;
    else {
        *f_pos += count;
        rc = count;

        printk(KERN_INFO "written %d bytes(s) from %d\n", count, p);
    }
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
    printk("hello%d %s():\n", MINOR(devp->cdev.dev), __FUNCTION__);
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
    .compat_ioctl = dev_ioctl,
    .llseek = dev_llseek,
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
        //hello_devp[i].cdev = cdev_alloc();
        cdev_init(&(hello_devp[i].cdev), &dev_fops);
        hello_devp[i].cdev.owner = THIS_MODULE;
        memset(hello_devp[i].mem, 0, GLOBALMEM_SIZE);
        rc = cdev_add(&(hello_devp[i].cdev), MKDEV(dev_major, i), 1);
        if (rc < 0) {
            printk("add chr dev failed\n");
            goto failed;
        }
    }

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
