#Automatically passed to sub-makes
MAKEFLAGS += --no-print-directory --no-builtin-rules
KERNEL_CONFIG_FILE=boot.conf
INITRAMFS_FILE=build/initramfs.cpio
LINUX_SRC_URL=https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.14.tar.xz 
LINUX_SRC_DIR=build/linux-5.14
KERNEL_BINARY_FILE=$(LINUX_SRC_DIR)/$(shell $(MAKE) --silent -C $(LINUX_SRC_DIR) image_name)
CONFIG_SET=$(LINUX_SRC_DIR)/scripts/config --file $(LINUX_SRC_DIR)/.config

copy-config = cp $(LINUX_SRC_DIR)/.config $(1)/$(2)-$(shell date --iso-8601=seconds).config

.PHONY: help
help:
	echo "build: Build the kernel and modules using .config in" $(LINUX_SRC_DIR)
	echo "download: Download the linux sources into directory" $(LINUX_SRC_DIR)

.PHONY: help-kernel
kernel-help:
	$(MAKE) -C $(LINUX_SRC_DIR) help | less
	
.PHONY: help-devices
help-devices:
	cat $(LINUX_SRC_DIR)/Documentation/admin-guide/devices.txt | less
	
.PHONY: help-initramfs
help-initramfs:
	build/linux-5.14/usr/gen_init_cpio
	
.PHONY: inspect-kernel
inspect-kernel:
	file --brief $(KERNEL_BINARY_FILE)

.PHONY: kbuild-docs
kbuild-docs:
	vim -M $(LINUX_SRC_DIR)/Documentation/kbuild/kbuild.rst

.PHONY: configure-kernel
configure-kernel:
	$(MAKE) -C $(LINUX_SRC_DIR) menuconfig	
	$(call copy-config,records,exit-menuconfig)

.PHONY: run
run: 
	qemu-system-x86_64 -nographic -enable-kvm -kernel $(KERNEL_BINARY_FILE) \
		-append "console=ttyS0"

.PHONY: build-all
build-all:
	$(MAKE) dirs
	$(MAKE) use-saved-config
	$(MAKE) gen-key
	$(MAKE) build-modules-initial
	$(MAKE) build-kernel-initial
	$(MAKE) build-init
	$(MAKE) initramfs
	$(MAKE) kernel-clean
	$(MAKE) use-saved-config
	$(MAKE) build

.PHONY: build
build:
	$(call copy-config,records,bzImage-build)
	$(CONFIG_SET) --set-str CONFIG_INITRAMFS_SOURCE "$(shell realpath initramfs.conf)"
	$(MAKE) -C $(LINUX_SRC_DIR) --jobs=4 bzImage
	echo "bzImage is located at" $(KERNEL_BINARY_FILE)

.PHONY: mk-test
mk-test:
	$(CONFIG_SET) --set-str CONFIG_INITRAMFS_SOURCE "$(shell realpath build/initramfs)"
	#echo "$(shell realpath build/initramfs)"
	
.PHONY: build-init
build-init:
	$(MAKE) -C sl-src
	cp --recursive sl-src/init build/initramfs/runtime/init

#Build the kernel without initramfs in order to generate modules.builtin
.PHONY: build-kernel-initial
build-kernel-initial:
	$(CONFIG_SET) --disable CONFIG_BOOT_CONFIG
	$(CONFIG_SET) --disable CONFIG_BLK_DEV_INITRD
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
	#(cd build/initramfs && find -depth -type f,d) | cpio --verbose --format=newc --create --owner=+0:+0\
	#	--device-independent --directory=build/initramfs --file=$(INITRAMFS_FILE)
	#Bootconfig won't compile, missing symbol "ret"
	#$(MAKE) -C $(LINUX_SRC_DIR)/tools/bootconfig
	#Add boot config (alternative to kernal command line args)
	#tools/bootconfig/bootconfig -a $(KERNEL_CONFIG_FILE) $(INITRAMFS_FILE)

#List files in current initramfs archive
.PHONY: list-initramfs
list-initramfs:
	cat $(LINUX_SRC_DIR)/usr/initramfs_data.cpio | cpio --list

#Estimate the size of all kernel modules
.PHONY: compute_modules_size 
compute_modules_size:
	#TODO use initramfs directory after module install instead
	find $(LINUX_SRC_DIR) -name "*.ko" | tr '\n' '\0' | du --summarize \
		--apparent-size --human-readable --total --files0-from=-
	
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
	rm -r build/initramfs build/kernel_key.pem
	
.PHONY: kernel-clean
kernel-clean:
	$(MAKE) -C $(LINUX_SRC_DIR) clean
	
.PHONY: dirs
dirs:
	mkdir --parents build/initramfs/runtime records
	
.DEFAULT:
	echo "Unsupported target"
