obj-m += myfs.o

myfs-objs := myfs_main.o myfs_node.o myfs_dir.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) clean 
