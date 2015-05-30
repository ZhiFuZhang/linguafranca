ifneq ($(KERNELRELEASE),)
	obj-m := ipshark.o
	ipshark-objs := core.o devset.o  ip_queue.o hook.o
	ifeq ($(DEBUG),y)
		ccflags-y += -DDEBUG
	endif
	ifeq ($(UNITTEST),y)
		ccflags-y += -DUNITTEST
	endif


else

CURRENT_PATH:=$(shell pwd)
LINUX_KERNEL_PATH:=/lib/modules/$(shell uname -r)/build
default:
	$(MAKE) -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean
install:
	./ips_load
uninstall:
	./ips_unload
endif
