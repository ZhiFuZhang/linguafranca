#
#	the code is compiled with linux kernel 3.10
#
ifneq ($(KERNELRELEASE),)
	obj-m :=nfstat.o
	nfstat-objs := nfs_hook.o ip_counter.o  ip_rule.o
	ifeq ($(DEBUG),y)
		ccflags-y += -DDEBUG
	endif
else

CURRENT_PATH:=$(shell pwd)
LINUX_KERNEL_PATH:=/lib/modules/$(shell uname -r)/build
default:
	$(MAKE) -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean
install:
	./nfs_load
uninstall:
	./nfs_unload
endif
