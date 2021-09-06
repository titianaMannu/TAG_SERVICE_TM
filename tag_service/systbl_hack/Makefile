

MODNAME=systbl_hack

ifeq ($(KERNELRELEASE),)
.PHONY: all install clean uninstall
all:
	$(MAKE)  -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
load:
	insmod ./$(MODNAME).ko
unload:
	rmmod ./$(MODNAME).ko

else
obj-m += $(MODNAME).o
$(MODNAME)-y := systbl_hack_main.o memory-mapper/virtual-to-phisical-memory-mapper.o systbl_hack_service.o

endif
