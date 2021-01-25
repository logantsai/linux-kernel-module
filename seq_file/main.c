#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/delay.h>

static struct proc_dir_entry *hello_dir, *hello_entry;

#define MAX_LINES 1000

static void *hello_seq_start(struct seq_file *s, loff_t *pos)
{
	printk("%s: pos:%lld\n", __func__, *pos);
	if (*pos >= MAX_LINES)
        return NULL;

    loff_t *spos = kmalloc(sizeof(loff_t), GFP_KERNEL);

    if (!spos)
        return NULL;

    *spos = *pos;

	return spos;

}

static void *hello_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    uint32_t *spos = v;
    *pos = ++(*spos);
    if (*pos >= MAX_LINES) {
        return NULL; // no more data to read
    }
	printk("%s: spos:%d\n", __func__, *spos);
	return spos;
}

static void hello_seq_stop(struct seq_file *s, void *v)
{
    kfree(v);
    /* nothing to do, we use a static value in start() */
}

static int hello_seq_show(struct seq_file *s, void *v)
{

	int n;

	n = *((int*)v);
	seq_printf(s, "lines:%d\n", n);
	return 0;

}


static struct seq_operations hello_seq_ops = {
	.start = hello_seq_start,
	.next  = hello_seq_next,
	.stop  = hello_seq_stop,
	.show  = hello_seq_show
};

static int hello_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &hello_seq_ops);
};

static struct file_operations hello_file_ops = {
	.owner = THIS_MODULE,
	.open = hello_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

int helloinit(void)
{
	hello_dir = proc_mkdir("hello_dir", NULL);
    hello_entry = proc_create_data("hello", 0666, hello_dir, &hello_file_ops, NULL);
	if (!hello_entry)
        printk(KERN_DEBUG "create proc entry fail");

    printk(KERN_DEBUG "hello world!\n");
    return 0;
}

void helloexit(void)
{
	remove_proc_entry("hello_entry", hello_dir);
	remove_proc_entry("hello_dir", NULL);
    printk(KERN_DEBUG "goodbye world!!\n");
}

module_init(helloinit);
module_exit(helloexit);

MODULE_LICENSE("GPL");
