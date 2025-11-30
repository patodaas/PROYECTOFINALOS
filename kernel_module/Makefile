obj-m += storage_stats.o

KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

EXTRA_CFLAGS += -msse -msse2 -mfpmath=sse -Wno-format-truncation -Wno-unused-parameter -Wno-missing-prototypes

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

install: all
	sudo insmod storage_stats.ko

uninstall:
	sudo rmmod storage_stats || true

reload: uninstall all install

test:
	cat /proc/storage_stats

.PHONY: all clean install uninstall reload test

