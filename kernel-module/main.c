#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
/*
    Standard types are:
      byte, short, ushort, int, uint, long, ulong
      charp: a character pointer
      bool: a bool, values 0/1, y/n, Y/N.
      invbool: the above, only sense-reversed (N = true).
*/

static char b_byte = 1;
module_param(b_byte, byte, S_IRUGO|S_IWUSR);

static short int b_short = 2;
module_param(b_short, short, S_IRUGO|S_IWUSR);

static unsigned short int b_ushort = 3;
module_param(b_ushort, ushort, S_IRUGO|S_IWUSR);

static int b_int = 6;
module_param(b_int, int, S_IRUGO|S_IWUSR);

static unsigned int b_uint = 5;
module_param(b_uint, uint, S_IRUGO|S_IWUSR);

static long b_long = 6;
module_param(b_long, long, S_IRUGO|S_IWUSR);

static unsigned long b_ulong = 7;
module_param(b_ulong, ulong, S_IRUGO|S_IWUSR);

static char *b_charp = "hello";
module_param(b_charp, charp, S_IRUGO|S_IWUSR);

static bool b_bool = 1;
module_param(b_bool, bool, S_IRUGO|S_IWUSR);

static unsigned int log_enable = 0x00;
static int log_enable_set(const char *arg, const struct kernel_param *kp)
{
    int ret = 0;
    unsigned long new_log_level;
    ret = kstrtoul(arg, 10, &new_log_level);
    if (ret)
        goto exit;
    if (new_log_level > 8 || new_log_level < 0) {
        pr_err("Out of recommand range %lu", new_log_level);
        ret = -EINVAL;
    }
    log_enable = new_log_level;
exit:
    return ret;
}
static const struct kernel_param_ops log_enable_ops = {
    .set = log_enable_set,
    .get = param_get_ulong,

};
module_param_cb(log_enable, &log_enable_ops, &log_enable, 0644);
MODULE_PARM_DESC(log_enable, "Setting log level");


int helloinit(void)
{
    printk(KERN_DEBUG "hello world!\n");
    printk("b_byte: %d\n", b_byte);
    printk("b_short: %d\n", b_short);
    printk("b_ushort: %u\n", b_ushort);
    printk("b_int: %d\n", b_int);
    printk("b_uint: %u\n", b_uint);
    printk("b_long: %ld\n", b_long);
    printk("b_ulong: %lu\n", b_ulong);
    printk("b_charp: %s\n", b_charp);
    printk("b_bool: %d\n", b_bool);
    return 0;
}

void helloexit(void)
{
    printk(KERN_DEBUG "goodbye world!!\n");
}
module_init(helloinit);
module_exit(helloexit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mark.tsai@quantatw.com");
MODULE_DESCRIPTION("a module template");
MODULE_VERSION("a.a.a");
MODULE_ALIAS("MODULE");
