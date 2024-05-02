# SPDX-License-Identifier: GPL-2.0
#
# Makefile for spfs.ko kernel module
#
# Copyright (c) 2023 Steve D. Pate

PWD   := $(shell pwd)
obj-m += spfs.o
spfs-objs := sp_alloc.o sp_dir.o sp_file.o sp_inode.o sp_ioctl.o
ccflags-y := -g

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
install:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules_install
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
