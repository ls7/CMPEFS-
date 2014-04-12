KERNEL_RELEASE = $(shell uname -r)
KERNEL_BUILDROOT ?= /lib/modules/$(KERNEL_RELEASE)/build
BUILDROOT = $(shell pwd)

TARGETS = cmpefs.ko
OBJECTS = cmpefs.o cmpefs.mod.c cmpefs.mod.o .cmpefs.ko.cmd .cmpefs.mod.o.cmd .cmpefs.o.cmd .tmp_versions modules.order Module.symvers

obj-m := cmpefs.o

all :
	$(MAKE) -C $(KERNEL_BUILDROOT) SUBDIRS=$(BUILDROOT) modules

clean :
	-rm -frv $(OBJECTS)

distclean : clean
	-rm -frv $(TARGETS)

test :
	umount /mnt || true
	rmmod cmpefs || true
	insmod $(BUILDROOT)/cmpefs.ko
	mount -t cmpefs none /mnt
	cd /mnt
	touch test.txt
	chmod 777 test.txt
	echo "Hello" > test.txt
	mkdir -p /mnt/d1/d2
	cd /
	umount /mnt
	rmmod cmpefs
	dmesg | grep -i --color=auto cmpefs


.PHONY : all clean distclean test
