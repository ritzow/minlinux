MAKEFLAGS += --silent
INITRAMFS_FILE=initramfs.cpio
LINUX_MAJOR=5
LINUX_VERSION=linux-5.14

copy-config =cp $(LINUX_VERSION)/.config $(1)/$(2)-$(shell date --iso-8601=seconds).config

.PHONY: help
help:
	echo "build: Build the kernel and modules using .config in" $(LINUX_VERSION)
	echo "download: Download the linux sources into directory" $(LINUX_VERSION)

kbuild-docs:
	vim -M linux-5.14/Documentation/kbuild/kbuild.rst

#Build the kernel
.PHONY: build
build: init-cpio
	$(MAKE) -C $(LINUX_VERSION) bzImage
	$(call copy-config,results,bzImage-build)
	echo "bzImage is located at" $(shell $(MAKE) -C $(LINUX_VERSION) image_name) 

.PHONY: build-modules
build-modules:
	#TODO use linux Makefile modules_install to put modules in initramfs
	$(call copy-config,results,modules-build)

#Create the initramfs archive file needed by build
.PHONY: init-cpio
init-cpio: build-modules
	$(MAKE) -C $(LINUX_VERSION) INSTALL_MOD_STRIP=1 MODLIB=../initramfs/modules #INSTALL_MOD_PATH=../initramfs modules_install
	(cd initramfs && find . -depth -type f) | cpio --verbose --format=newc --create --owner=+0:+0\
		--device-independent --directory=initramfs --file=$(INITRAMFS_FILE)

#List files in current initramfs archive
.PHONY: list-cpio
list-cpio:
	cat $(INITRAMFS_FILE) | cpio --list

#Estimate the size of all kernel modules
.PHONY: compute_modules_size 
compute_modules_size:
	find -name "*.ko" | tr '\n' '\0' | du --files0-from=- --total --bytes

#Download the kernel source code
.PHONY: download
download: clean
	curl -s https://cdn.kernel.org/pub/linux/kernel/v$(LINUX_MAJOR).x/$(LINUX_VERSION).tar.xz | tar --extract --xz -f -

#Create a backu point of the current .config
.PHONY: backup-config
backup-config:
	$(call copy-config,records,backup)

.PHONY: clean
clean:
	rm $(INITRAMFS_FILE)
