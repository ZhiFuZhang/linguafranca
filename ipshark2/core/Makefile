ifneq ($(KERNELRELEASE),)

	obj-m := ipshark2.o
	ifeq ($(UNITTEST),y)
		ipshark2-objs := ut/core.o ut/devset.o ut/ip_queue.o
	else
		ipshark2-objs := core.o devset.o ip_queue.o
	endif

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
	./ips2_load
uninstall:
	./ips2_unload
endif
