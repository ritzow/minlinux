INITRAMFS_FILE=initramfs.cpio
LINUX_VERSION=linux-5.14

.PHONY: help
help:
	@echo "build: Build the kernel and modules using .config in" $(LINUX_VERSION)
	@echo "download: Download the linux sources into directory" $(LINUX_VERSION)

.PHONY: build
build: init-cpio
	$(MAKE) -C $(LINUX_VERSION) all

.PHONY: init-cpio
init-cpio:
	@(cd initramfs && find . -depth -type f) | cpio --verbose --format=newc --create --owner=+0:+0\
		--device-independent --directory=initramfs --file=$(INITRAMFS_FILE)

.PHONY: list-cpio
list-cpio:
	@cat $(INITRAMFS_FILE) | cpio --list

.PHONY: download
download: clean
	@curl -s https://cdn.kernel.org/pub/linux/kernel/v5.x/$(LINUX_VERSION).tar.xz | tar --extract --xz -f -

.PHONY: copy_config
backup-config:
	cp $(LINUX_VERSION)/.config backup-$(shell date --iso-8601=seconds).config

.PHONY: clean
clean:
	rm $(INITRAMFS_FILE)
