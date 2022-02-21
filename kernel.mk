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

.PHONY: imgfile
imgfile:
	@echo $(abspath $(KERNEL_BINARY_FILE))

.PHONY: configure-kernel
configure-kernel:
	$(MAKE) -C $(LINUX_SRC_DIR) menuconfig	
	$(call copy-config,records,exit-menuconfig)

.PHONY: run
run: 
	qemu-system-x86_64 -vga none -nographic -sandbox on \
	-cpu Icelake-Server-v4 \
	-kernel $(KERNEL_BINARY_FILE) -append "console=ttyS0" 

.PHONY: run-efi
run-efi: 
	qemu-system-x86_64 -vga none -nographic -sandbox on \
	-kernel $(KERNEL_BINARY_FILE) -append "console=ttyS0" -bios /usr/share/ovmf/OVMF.fd

.PHONY: buildrun
buildrun: build
	$(MAKE) run

.PHONY: kernel
kernel: $(KERNEL_BINARY_FILE)

#The real build target
$(KERNEL_BINARY_FILE): $(LINUX_SRC_DIR) $(BUILD_DIR)/kernel_key.pem 
	$(MAKE) build-kernel-initial

.PHONY: build-all
build-all:
	$(MAKE) use-saved-config
	$(MAKE) gen-key
	$(MAKE) build-kernel-initial
	$(MAKE) build-modules
	$(MAKE) use-saved-config
	$(MAKE) install-kernel-headers
	$(MAKE) build

.PHONY: build
build: init.proj build/initramfs.conf
	$(MAKE) -C $(LINUX_SRC_DIR) --jobs=4 bzImage

$(BUILD_DIR)/initramfs.conf: $(PROJECT_DIR)/initramfs.conf kernel.mk
	(cat initramfs.conf; $(MAKE) _gen-cpio-list) > build/initramfs.conf 

# Generate lines in the cpio archive specification
.PHONY: _gen-cpio-list
.ONESHELL: 
_gen-cpio-list:
	@echo "\n"
	@find build/initramfs/* | while read -r LINE; do
		if test -d $$LINE; then
			echo "dir" $${LINE#build/initramfs} 755 0 0
		elif test -f $$LINE; then
			echo "file" $${LINE#build/initramfs} $$(realpath $$LINE) 755 0 0
		fi
	done

.PHONY: install-kernel-headers
install-kernel-headers:
	$(MAKE) -C $(LINUX_SRC_DIR) INSTALL_HDR_PATH=.. headers_install
	cp -r $(LINUX_SRC_DIR)/tools/include/nolibc build/include

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

$(BUILD_DIR)/kernel_key.pem:
	openssl req -new -utf8 -sha256 -days 36500 -batch -x509 \
		-config $(PROJECT_DIR)/kernel_keygen.conf -outform PEM -keyout $(BUILD_DIR)/kernel_key.pem -out $(BUILD_DIR)/kernel_key.pem

gitclone=git clone --single-branch --depth 1 --branch $(1) $(3) $(2)

$(BUILD_DIR)/manual:
	$(MAKE) -C man-pages -j install prefix=$(BUILD_DIR)/manual

#Create a backup point of the current .config
.PHONY: backup-config
backup-config: dirs
	$(call copy-config,records,backup)
	
save-new-config:
	cp $(LINUX_SRC_DIR)/.config kernel.config
	
#Diff the current git tracked kernel.config with the one used by linux build targets
.PHONY: diff-config
diff-config:
	@diff --color=always --speed-large-files \
		-s kernel.config $(LINUX_SRC_DIR)/.config | less -r --quit-if-one-screen --quit-at-eof
	
.PHONY: use-saved-config
use-saved-config:
	$(call copy-config,records,pre-use-backup) || true
	cp kernel.config $(LINUX_SRC_DIR)/.config

.PHONY: clean
clean:
	$(call copy-config,records,pre-clean)
	$(MAKE) -C $(LINUX_SRC_DIR) clean
	rm -rf $(BUILD_DIR)
#	rm -r build/initramfs build/kernel_key.pem $(LINUX_SRC_DIR)
	
.PHONY: viewman
viewman:
	@man --manpath=build/manual/share/man $(page) || true
