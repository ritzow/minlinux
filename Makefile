#Automatically passed to sub-makes
MAKEFLAGS += --no-builtin-rules #--silent
KERNEL_CONFIG_FILE=boot.conf
INITRAMFS_FILE=build/initramfs.cpio
LINUX_SRC_URL=https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.14.tar.xz 
LINUX_SRC_DIR=build/linux-5.14
KERNEL_BINARY_FILE=$(LINUX_SRC_DIR)/$(shell $(MAKE) --silent -C $(LINUX_SRC_DIR) image_name)

copy-config = cp $(LINUX_SRC_DIR)/.config $(1)/$(2)-$(shell date --iso-8601=seconds).config

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
	qemu-system-x86_64 -nographic -enable-kvm -kernel $(KERNEL_BINARY_FILE) -append "rootfstype=initramfs console=ttyS0"

#Build the kernel, includes the cpio initramfs in bzImage
.PHONY: build
build:
	$(MAKE) use-saved-config
	$(call copy-config,records,bzImage-build)
	$(MAKE) -C $(LINUX_SRC_DIR) --jobs=4 bzImage
	echo "bzImage is located at" $(KERNEL_BINARY_FILE) 

.PHONY: build-all
build-all:
	$(MAKE) dirs
	$(MAKE) use-saved-config
	$(MAKE) build-modules-initial
	$(MAKE) gen-key
	$(MAKE) build-init
	$(MAKE) build-kernel-initial
	$(MAKE) initramfs
	#TODO need to restore use-saved-config so it doesn't ask questions here
	$(MAKE) build

.PHONY: build-init
build-init:
	$(MAKE) -C sl-src
	cp --recursive sl-src/server-linux build/initramfs/runtime/server-linux

#Build the kernel without initramfs in order to generate modules.builtin
.PHONY: build-kernel-initial
build-kernel-initial:
	$(LINUX_SRC_DIR)/scripts/config --file $(LINUX_SRC_DIR)/.config --disable CONFIG_BOOT_CONFIG
	$(LINUX_SRC_DIR)/scripts/config --file $(LINUX_SRC_DIR)/.config --disable CONFIG_BLK_DEV_INITRD
	$(MAKE) -C $(LINUX_SRC_DIR) --jobs=4 vmlinux

#Build the modules without installing them in the initramfs
.PHONY: build-modules-initial
build-modules-initial:
	$(MAKE) -C $(LINUX_SRC_DIR) --jobs=4 modules

.PHONY: gen-key
gen-key:
	openssl req -new -utf8 -sha256 -days 36500 -batch -x509 \
		-config kernel_keygen.conf -outform PEM -keyout build/kernel_key.pem -out build/kernel_key.pem

#Install modules in the initramfs directory and build the cpio archive
.PHONY: initramfs
initramfs: 
	#Call beforehand: build-kernel-initial build-modules-initial build-server-linux
	$(MAKE) -C $(LINUX_SRC_DIR) INSTALL_MOD_PATH=../initramfs modules_install #INSTALL_MOD_STRIP=1 
	(cd build/initramfs && find -depth -type f,d) | cpio --verbose --format=newc --create --owner=+0:+0\
		--device-independent --directory=build/initramfs --file=$(INITRAMFS_FILE)
	#Bootconfig won't compile, missing symbol "ret"
	#$(MAKE) -C $(LINUX_SRC_DIR)/tools/bootconfig
	#Add boot config (alternative to kernal command line args)
	#tools/bootconfig/bootconfig -a $(KERNEL_CONFIG_FILE) $(INITRAMFS_FILE)

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
	curl -s $(LINUX_SRC_URL) | tar --extract --xz --directory build -f -

#Create a backup point of the current .config
.PHONY: backup-config
backup-config: dirs
	$(call copy-config,records,backup)
	
save-new-config:
	cp $(LINUX_SRC_DIR)/.config kernel.config
	
.PHONY: get-config
get-config:
	cp $(LINUX_SRC_DIR)/.config kernel.config
	
.PHONY: use-saved-config
use-saved-config:
	$(call copy-config,records,pre-use-backup) || true
	cp kernel.config $(LINUX_SRC_DIR)/.config
	#$(MAKE) -C $(LINUX_SRC_DIR) oldconfig

.PHONY: clean
clean:
	$(call copy-config,records,pre-clean)
	$(MAKE) -C $(LINUX_SRC_DIR) clean
	rm -r build/initramfs build/kernel_key.pem
	
.PHONY: dirs
dirs:
	mkdir --parents build/initramfs/runtime records
	
.DEFAULT:
	echo "Unsupported target"
