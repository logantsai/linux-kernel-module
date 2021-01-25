#linux kernel path
KERNELDIR ?= /home/mark/Desktop/vexpress/linux/linux-5.4.2

PWD := $(shell pwd)

CC = $(CROSS_COMPILE)gcc

test_proc = recv_uevent

#module name
module_name := netlink-test

#This will generate kernel_module.ko
obj-m := $(module_name).o

#This module's other component
$(module_name)-objs := main.o

default: $(test_proc)
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

$(test_proc): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean \
	&& rm $(test_proc)
