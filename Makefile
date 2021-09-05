#Automatically passed to sub-makes
MAKEFLAGS += --silent --jobs=4 --no-builtin-rules
KERNEL_CONFIG_FILE=boot.conf
INITRAMFS_FILE=build/initramfs.cpio
LINUX_SRC_URL=https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.14.tar.xz 
LINUX_SRC_DIR=build/linux-5.14

copy-config =cp $(LINUX_SRC_DIR)/.config $(1)/$(2)-$(shell date --iso-8601=seconds).config

.PHONY: help
help:
	echo "build: Build the kernel and modules using .config in" $(LINUX_SRC_DIR)
	echo "download: Download the linux sources into directory" $(LINUX_SRC_DIR)

.PHONY: kbuild-docs
kbuild-docs:
	vim -M $(LINUX_SRC_DIR)/Documentation/kbuild/kbuild.rst

.PHONY: configure-kernel
configure-kernel:
	$(MAKE) -C $(LINUX_SRC_DIR) menuconfig	
	$(call copy-config,records,exit-menuconfig)

.PHONY: run
run: 
	#build
	qemu-system-x86_64 -enable-kvm -kernel $(LINUX_SRC_DIR)/$(shell $(MAKE) --silent -C $(LINUX_SRC_DIR) image_name)

#Build the kernel
.PHONY: build
build: 
	#initramfs
	$(MAKE) -C $(LINUX_SRC_DIR) bzImage
	$(call copy-config,records,bzImage-build)
	echo "bzImage is located at" $(shell $(MAKE) -C $(LINUX_SRC_DIR) image_name) 

.PHONY: build-kernel
build-kernel:
	$(MAKE) -C $(LINUX_SRC_DIR) bzImage

.PHONY: build-modules
build-modules:
	#TODO use linux Makefile modules_install to put modules in initramfs
	$(MAKE) -C $(LINUX_SRC_DIR) modules
	$(MAKE) -C $(LINUX_SRC_DIR) INSTALL_MOD_STRIP=1 INSTALL_MOD_PATH=../initramfs modules_install #MODLIB=../initramfs modules_install #INSTALL_MOD_PATH=../initramfs modules_install
	$(call copy-config,records,modules-build)

#Create the initramfs archive file needed by build
.PHONY: initramfs
initramfs: 
	#build-modules
	(cd build/initramfs && find . -depth -type f) | cpio --verbose --format=newc --create --owner=+0:+0\
		--device-independent --directory=build/initramfs --file=$(INITRAMFS_FILE)
	$(MAKE) -C $(LINUX_SRC_DIR)/tools/bootconfig
	#Add boot config (alternative to kernal command line args)
	tools/bootconfig/bootconfig -a $(KERNEL_CONFIG_FILE) $(INITRAMFS_FILE)

#List files in current initramfs archive
.PHONY: list-initramfs
list-initramfs:
	cat $(INITRAMFS_FILE) | cpio --list

#Estimate the size of all kernel modules
.PHONY: compute_modules_size 
compute_modules_size:
	#TODO use initramfs directory after module install instead
	find $(LINUX_SRC_DIR) -name "*.ko" | tr '\n' '\0' | du --summarize --apparent-size --human-readable --total --files0-from=-

#Download the kernel source code
.PHONY: download
download: dirs
	mkdir --parents build	
	curl -s $(LINUX_SRC_URL) | tar --extract --xz --directory build -f -

#Create a backu point of the current .config
.PHONY: backup-config
backup-config: dirs
	$(call copy-config,records,backup)

.PHONY: clean
clean:
	rm $(INITRAMFS_FILE)
	
.PHONY: dirs
dirs:
	mkdir --parents build/initramfs records
