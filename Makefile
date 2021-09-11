#Automatically passed to sub-makes
MAKEFLAGS += --no-print-directory --no-builtin-rules --no-builtin-variables
KERNEL_CONFIG_FILE=boot.conf
LINUX_SRC_URL=https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.14.2.tar.xz
LINUX_SRC_DIR=build/linux-5.14.2
KERNEL_BINARY_FILE=$(LINUX_SRC_DIR)/$(shell $(MAKE) --silent -C $(LINUX_SRC_DIR) image_name)
CONFIG_SET=$(LINUX_SRC_DIR)/scripts/config --file $(LINUX_SRC_DIR)/.config

copy-config = cp $(LINUX_SRC_DIR)/.config $(1)/$(2)-$(shell date --iso-8601=seconds).config

.PHONY: help
help:
	echo "build: Build the kernel and modules using .config in" $(LINUX_SRC_DIR)
	echo "download: Download the linux sources into directory" $(LINUX_SRC_DIR)

.PHONY: help-kernel
help-kernel:
	$(MAKE) -C $(LINUX_SRC_DIR) help | less
	
.PHONY: help-devices
help-devices:
	cat $(LINUX_SRC_DIR)/Documentation/admin-guide/devices.txt | less
	
.PHONY: help-initramfs
help-initramfs:
	build/linux-5.14/usr/gen_init_cpio
	
.PHONY: inspect-kernel
inspect-kernel:
	@file --brief $(KERNEL_BINARY_FILE)
	@stat --printf="%s" $(KERNEL_BINARY_FILE) 
	@echo " bytes"

.PHONY: kbuild-docs
kbuild-docs:
	vim -M $(LINUX_SRC_DIR)/Documentation/kbuild/kbuild.rst

.PHONY: configure-kernel
configure-kernel:
	$(MAKE) -C $(LINUX_SRC_DIR) menuconfig	
	$(call copy-config,records,exit-menuconfig)

.PHONY: run
run: 
	qemu-system-x86_64 -nographic -kernel $(KERNEL_BINARY_FILE) \
		-append "console=ttyS0"

.PHONY: build-all
build-all:
	$(MAKE) dirs
	$(MAKE) download
	$(MAKE) use-saved-config
	$(MAKE) gen-key
	$(MAKE) build-kernel-initial
	$(MAKE) build-modules
	$(MAKE) use-saved-config
	$(MAKE) build

.PHONY: build
build:
	$(MAKE) build-init
	$(call copy-config,records,bzImage-build)
	$(CONFIG_SET) --set-str CONFIG_INITRAMFS_SOURCE "$(shell realpath initramfs.conf)"
	$(MAKE) -C $(LINUX_SRC_DIR) --jobs=4 bzImage
	echo "bzImage is located at" $(KERNEL_BINARY_FILE)

.PHONY: install-kernel-headers
install-kernel-headers:
	$(MAKE) -C $(LINUX_SRC_DIR) INSTALL_HDR_PATH=.. headers_install
	cp -r $(LINUX_SRC_DIR)/tools/include/nolibc build/include

LINUX_INCLUDES = "$(shell realpath build/include)"
export LINUX_INCLUDES

.PHONY: build-init
build-init:
	$(MAKE) -C sl-src init

.PHONY: apt-install-reqs
apt-install-reqs:
	sudo apt-get install dash flex bison libssl-dev libelf-dev bc zstd

#Build the kernel without initramfs in order to generate modules.builtin
.PHONY: build-kernel-initial
build-kernel-initial:
	$(CONFIG_SET) --disable CONFIG_BOOT_CONFIG
	$(CONFIG_SET) --disable CONFIG_BLK_DEV_INITRD
	$(MAKE) -C $(LINUX_SRC_DIR) --jobs=4 vmlinux

#Build the modules without installing them in the initramfs
.PHONY: build-modules
build-modules-initial:
	$(MAKE) -C $(LINUX_SRC_DIR) --jobs=4 modules
	$(MAKE) -C $(LINUX_SRC_DIR) INSTALL_MOD_PATH=../initramfs modules_install #INSTALL_MOD_STRIP=1 
	#TODO generate initramfs.conf with all modules and modprobe included
	@#Bootconfig won't compile, missing symbol "ret"
	@#$(MAKE) -C $(LINUX_SRC_DIR)/tools/bootconfig
	@#Add boot config (alternative to kernal command line args)
	@#tools/bootconfig/bootconfig -a $(KERNEL_CONFIG_FILE) $(INITRAMFS_FILE)

.PHONY: gen-key
gen-key:
	openssl req -new -utf8 -sha256 -days 36500 -batch -x509 \
		-config kernel_keygen.conf -outform PEM -keyout build/kernel_key.pem -out build/kernel_key.pem

#List files in current initramfs archive
.PHONY: list-initramfs
list-initramfs:
	cat $(LINUX_SRC_DIR)/usr/initramfs_data.cpio | cpio --list
	
#Download the kernel source code
.PHONY: download
download: dirs
	curl -s $(LINUX_SRC_URL) | tar --extract --xz --directory build -f -

#Create a backup point of the current .config
.PHONY: backup-config
backup-config: dirs
	$(call copy-config,records,backup)
	
save-new-config:
	cp $(LINUX_SRC_DIR)/.config kernel.config
	
.PHONY: diff-config
diff-config:
	diff --color --speed-large-files -s kernel.config $(LINUX_SRC_DIR)/.config
	
.PHONY: use-saved-config
use-saved-config:
	$(call copy-config,records,pre-use-backup) || true
	cp kernel.config $(LINUX_SRC_DIR)/.config
	#$(MAKE) -C $(LINUX_SRC_DIR) oldconfig

.PHONY: clean
clean:
	$(call copy-config,records,pre-clean)
	$(MAKE) -C $(LINUX_SRC_DIR) clean
	rm -r build/initramfs build/kernel_key.pem $(LINUX_SRC_DIR)
	
.PHONY: kernel-clean
kernel-clean:
	$(MAKE) -C $(LINUX_SRC_DIR) clean
	
.PHONY: dirs
dirs:
	mkdir --parents build/initramfs records
	
.DEFAULT:
	echo "Unsupported target"
