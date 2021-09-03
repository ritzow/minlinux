INITRAMFS_FILE=initramfs.cpio.lz4

dummy:
	@echo "Dummy!"

init-cpio:
	@(cd initramfs && find . -depth -type f) | cpio --verbose --format=newc --create --owner=+0:+0\
		--device-independent --directory=initramfs | lz4 --force --compress - $(INITRAMFS_FILE)

list-cpio:
	@lz4 --decompress --to-stdout $(INITRAMFS_FILE) | cpio --list

download: clean
	@curl -s https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.14.tar.xz | tar --extract --xz -f -

.PHONY: copy_config
backup-config:
	cp linux-5.14/.config backup-$(shell date --iso-8601=seconds).config

clean:
	rm $(INITRAMFS_FILE)
