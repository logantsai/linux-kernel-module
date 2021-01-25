#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/of.h>

int helloinit(void)
{
	/* device node pointer */
	struct device_node *dn = of_find_node_by_path("/hello");
	struct device_node *child_dn;
	int int_array[4];
	int rc, iter;
    u32 level;
    struct property *prop;

	if (!dn) {
		printk(KERN_DEBUG "Get device node hello fail\n");
	}

	char *name = of_node_full_name(dn);
		printk("Device node name is: %s\n", name);

	for_each_child_of_node(dn, child_dn) {
		char *t_name = of_node_full_name(child_dn);
		printk("Device node name is: %s\n", t_name);
	}

	if (of_property_count_elems_of_size(dn, "array", 4)) {
		rc = of_property_read_u32_array(dn, "array", int_array, 4);
		for (iter = 0; iter < 4; iter++) {
			printk("array[%d]: %d\n", iter, int_array[iter]);
		}
	}

    of_property_read_u32(dn, "level", &level);
    printk("level: %d", level);


    of_property_for_each_string(dn, "ary-string", prop, name) {
        printk("string: %s\n", name);
    }
	return 0;
}

void helloexit(void)
{
	printk(KERN_DEBUG "goodbye world!!\n");
}

module_init(helloinit);
module_exit(helloexit);

MODULE_LICENSE("GPL");
