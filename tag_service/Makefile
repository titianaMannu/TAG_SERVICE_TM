MODNAME=tag_service

ifeq ($(KERNELRELEASE),)
.PHONY: all install clean uninstall
all:
	cd systbl_hack && $(MAKE)
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	cd systbl_hack && $(MAKE) clean
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
else
obj-m += $(MODNAME).o
$(MODNAME)-y := tag_main.o tag.o
KBUILD_EXTRA_SYMBOLS := $(PWD)/systbl_hack/Module.symvers
endif
