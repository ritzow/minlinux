#Automatically passed to sub-makes
MAKEFLAGS += --no-print-directory --no-builtin-rules --no-builtin-variables
KERNEL_CONFIG_FILE=boot.conf
LINUX_SRC_URL=https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.14.7.tar.xz
LINUX_SRC_DIR=build/linux-5.14.7
KERNEL_BINARY_FILE=$(LINUX_SRC_DIR)/$(shell $(MAKE) --silent -C $(LINUX_SRC_DIR) image_name)
CONFIG_SET := $(LINUX_SRC_DIR)/scripts/config --file $(LINUX_SRC_DIR)/.config

copy-config = cp $(LINUX_SRC_DIR)/.config $(1)/$(2)-$(shell date --iso-8601=seconds).config

.PHONY: help
help:
	@echo "build: Build the kernel and modules using .config in" $(LINUX_SRC_DIR)
	@echo "download: Download the linux sources into directory" $(LINUX_SRC_DIR)

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

.PHONY: configure-kernel
configure-kernel:
	$(MAKE) -C $(LINUX_SRC_DIR) menuconfig	
	$(call copy-config,records,exit-menuconfig)

.PHONY: run
run: 
	qemu-system-x86_64 -vga none -nographic -sandbox on \
	-kernel $(KERNEL_BINARY_FILE) -append "console=ttyS0"

.PHONY: run-efi
run-efi: 
	qemu-system-x86_64 -vga none -nographic -sandbox on \
	-kernel $(KERNEL_BINARY_FILE) -append "console=ttyS0" -bios /usr/share/ovmf/OVMF.fd

.PHONY: buildrun
buildrun: build
	$(MAKE) run

.PHONY: build-all
build-all:
	$(MAKE) dirs
	$(MAKE) download-kernel
	$(MAKE) use-saved-config
	$(MAKE) gen-key
	$(MAKE) build-kernel-initial
	$(MAKE) build-modules
	$(MAKE) use-saved-config
	$(MAKE) install-kernel-headers
	$(MAKE) build

.PHONY: build
build: build-init
	$(call copy-config,records,bzImage-build)
	(cat initramfs.conf; $(MAKE) _gen-cpio-list) > build/initramfs.conf 
	$(MAKE) -C $(LINUX_SRC_DIR) --jobs=4 bzImage
	echo "bzImage is located at" $(KERNEL_BINARY_FILE)

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

.PHONY: build-init
build-init:
	$(MAKE) -C minlinux
	$(MAKE) -C init MUSL_DIR=$(abspath build/musl) BUILD_DIR="$(abspath build)" init

.PHONY: apt-install-kernel-reqs
apt-install-kernel-reqs:
	apt-get install dash flex bison libssl-dev libelf-dev libncurses-dev bc zstd

.PHONY: apt-install-proj-reqs
apt-install-proj-reqs:
	apt-get install curl openssl

#Build the kernel without initramfs in order to generate modules.builtin
.PHONY: build-kernel-initial
build-kernel-initial:
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

.PHONY: gen-key
gen-key:
	openssl req -new -utf8 -sha256 -days 36500 -batch -x509 \
		-config kernel_keygen.conf -outform PEM -keyout build/kernel_key.pem -out build/kernel_key.pem

#List files in current initramfs archive
.PHONY: list-initramfs
list-initramfs:
	@cat $(LINUX_SRC_DIR)/usr/initramfs_data.cpio | cpio --list
	
#Download the kernel source code
.PHONY: download-kernel
download-kernel: dirs
	curl -s $(LINUX_SRC_URL) | tar --extract --xz --directory build -f -

gitclone=git clone --single-branch --depth 1 --branch $(1) $(3) $(2)

.PHONY: download-manual
 download-manual:
	$(call gitclone,master,build/man-src,git://git.kernel.org/pub/scm/docs/man-pages/man-pages.git)
	$(MAKE) -C build/man-src -j install prefix=../manual
	rm -r build/man-src

.PHONY: download-jemalloc
download-jemalloc:
#git clone doesn't have ./configure script, requires autoconf
#git clone --depth 1 --single-branch --branch master \
#	https://github.com/jemalloc/jemalloc.git build/jemalloc
	curl -L -s https://github.com/jemalloc/jemalloc/releases/download/5.2.1/jemalloc-5.2.1.tar.bz2 | \
		tar --bzip2 --extract --directory build --file=-

.PHONY: download-musl
download-musl:
	$(call gitclone,master,build/musl-src,git://git.musl-libc.org/musl)

.PHONY: build-musl
build-musl:
	(cd build/musl-src; CC=gcc ./configure --disable-shared --prefix=$(abspath build/musl))
	$(MAKE) -C build/musl-src install

.PHONY: download-kmod
download-kmod:
#git clone --depth 1 --single-branch --branch master git://git.kernel.org/pub/scm/utils/kernel/kmod/kmod.git build/kmod-src
	curl https://mirrors.edge.kernel.org/pub/linux/utils/kernel/kmod/kmod-29.tar.xz | \
		tar --extract --xz --directory build -f -

.PHONY: build-kmod
build-kmod:	
#(cd build/kmod-29; ./configure CC=$(abspath build/musl/bin/musl-gcc) CFLAGS="-Os -nostdlib \
#	-I $(abspath build/musl/include) $(abspath build/musl/lib/libc.a)" --prefix=$(abspath build/kmod))
	(cd build/kmod-29; ./configure CC=$(abspath build/musl/bin/musl-gcc) CFLAGS="-Os -I $(abspath build/musl/include)" --enable-static --prefix=$(abspath build/kmod) --disable-tools --with-zstd)
	$(MAKE) -C build/kmod-29 install

.PHONY: download-zstd
download-zstd:
	$(call gitclone,dev,build/zstd-src,https://github.com/facebook/zstd.git)

.PHONY: build-zstd
build-zstd:
	$(shell $(MAKE) -C build/zstd-src/lib CC=$(abspath build/musl/bin/musl-gcc) \
		ZSTD_NO_UNUSED_FUNCTIONS=0 ZSTD_LIB_MINIFY=1 \
		ZSTD_LEGACY_SUPPORT=0 ZSTD_LIB_COMPRESSION=0 ZSTD_LIB_DEPRECATED=0 \
		PREFIX=$(abspath build/zstd) install)

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
	rm -r build/initramfs build/kernel_key.pem $(LINUX_SRC_DIR)
	
.PHONY: kernel-clean
kernel-clean:
	$(MAKE) -C $(LINUX_SRC_DIR) clean
	
.PHONY: dirs
dirs:
	mkdir --parents build/initramfs records
	
.PHONY: viewman
viewman:
	@man --manpath=build/manual/share/man $(page) || true

.DEFAULT:
	@echo "Unsupported target"
