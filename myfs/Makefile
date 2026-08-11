obj-m += myfs.o

myfs-objs := myfs_main.o myfs_node.o myfs_dir.o myfs_file.o

KDIR := /lib/modules/$(shell uname -r)/build
BUILD_DIR := $(CURDIR)/../build

all:
	make -C $(KDIR) M=$(CURDIR) MO=$(BUILD_DIR) modules

clean:
	make -C $(KDIR) M=$(CURDIR) MO=$(BUILD_DIR) clean 
