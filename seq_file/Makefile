#linux kernel path
KERNELDIR ?= /home/mark/Desktop/vexpress/linux/linux-5.4.2

PWD := $(shell pwd)

#module name
module_name := seq_file

#This will generate kernel_module.ko
obj-m := $(module_name).o

#This module's other component
$(module_name)-objs := main.o

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
