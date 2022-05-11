LINUX_SRC_DIR=$(PROJECT_DIR)/linux

echo $(LINUX_SRC_DIR)

$(MAKE) -C $(LINUX_SRC_DIR) oldconfig && $(MAKE) -C $(LINUX_SRC_DIR) prepare

KERNEL_BINARY_FILE := $(LINUX_SRC_DIR)/$(shell $(MAKE) --silent -C $(LINUX_SRC_DIR) image_name)
copy-config = cp $(LINUX_SRC_DIR)/.config $(1)/$(2)-$(shell date --iso-8601=seconds).config
CONFIG_SET := $(LINUX_SRC_DIR)/scripts/config --file $(LINUX_SRC_DIR)/.config

.PHONY: help-kernel
help-kernel:
	$(MAKE) -C $(LINUX_SRC_DIR) help | less
	
.PHONY: help-devices
help-devices:
	@cat $(LINUX_SRC_DIR)/Documentation/admin-guide/devices.txt | less
	
.PHONY: help-initramfs
help-initramfs:
	$(LINUX_SRC_DIR)/usr/gen_init_cpio

.PHONY: help-syscalls
help-syscalls:
	less $(LINUX_SRC_DIR)/arch/x86/entry/syscalls/syscall_64.tbl build/include/asm/unistd_64.h
	
.PHONY: inspect-kernel
inspect-kernel:
	@file --brief $(KERNEL_BINARY_FILE)
	@stat --printf="%s" $(KERNEL_BINARY_FILE) 
	@echo " bytes"

.PHONY: kbuild-docs
kbuild-docs:
	less $(LINUX_SRC_DIR)/Documentation/kbuild/kbuild.rst


.PHONY: build-all
build-all:
	$(MAKE) use-saved-config
	$(MAKE) gen-key
	$(MAKE) build-kernel-initial
	$(MAKE) build-modules
	$(MAKE) use-saved-config
	$(MAKE) install-kernel-headers
	$(MAKE) build

.PHONY: apt-install-kernel-reqs
apt-install-kernel-reqs:
	apt-get install dash flex bison libssl-dev libelf-dev libncurses-dev bc zstd

.PHONY: apt-install-proj-reqs
apt-install-proj-reqs:
	apt-get install curl openssl

#Build the kernel without initramfs in order to generate modules.builtin
.PHONY: build-kernel-initial
build-kernel-initial: $(LINUX_SRC_DIR)
	$(CONFIG_SET) --disable CONFIG_BOOT_CONFIG
	$(CONFIG_SET) --disable CONFIG_BLK_DEV_INITRD
	$(MAKE) -C $(LINUX_SRC_DIR) --jobs=4 vmlinux

#Build the modules without installing them in the initramfs
.PHONY: build-modules
build-modules:
	$(MAKE) -C $(LINUX_SRC_DIR) --jobs=4 modules
	$(MAKE) -C $(LINUX_SRC_DIR) INSTALL_MOD_PATH=../initramfs modules_install #INSTALL_MOD_STRIP=1 

.PHONY: build-bootconfig
build-bootconfig:
	$(MAKE) -C $(LINUX_SRC_DIR)/tools/bootconfig
# TODO use provided scripts to incldue bootconfig in initramfs
# https://wiki.gentoo.org/wiki/Custom_Initramfs
# tools/bootconfig/bootconfig -a $(KERNEL_CONFIG_FILE) $(INITRAMFS_FILE)