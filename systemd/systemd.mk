include $(COMMON_PRE_MK)

CROSS_DIR=$(BUILD_DIR)/compiler

.PHONY: systemd
systemd: glibc
	@echo SYSTEMD

.PHONY: glibc
glibc: $(CROSS_DIR)/binutils $(BUILD_DIR)/glibc
	mkdir -p $(BUILD_DIR)/glibc-build $(BUILD_DIR)/glibc-install
	(cd $(BUILD_DIR)/glibc-build; $(BUILD_DIR)/glibc/configure \
		CC="gcc" CFLAGS="-O3" \
		--srcdir=$(BUILD_DIR)/glibc \
		--prefix=$(BUILD_DIR)/glibc-install \
		--enable-kernel=5.14.7 \
		--with-headers=$(abspath $(BUILD_DIR)/include))
	$(MAKE) -C $(BUILD_DIR)/glibc-build

$(BUILD_DIR)/glibc:
	$(call git,git://sourceware.org/git/glibc.git,release/2.34/master,glibc)

$(CROSS_DIR):
	mkdir -p $(CROSS_DIR)

$(CROSS_DIR)/binutils: $(CROSS_DIR)
	$(call git,git://sourceware.org/git/binutils-gdb.git,binutils-2_37,$(CROSS_DIR)/binutils)

include $(COMMON_POST_MK)
