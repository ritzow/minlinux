PROJECT_DIR = $(abspath .)
COMMON_PRE_MK = $(PROJECT_DIR)/make/common.pre.mk

export PROJECT_DIR COMMON_PRE_MK

.PHONY: init
init:
	@$(MAKE) -C init -f init.mk init

.PHONY: util
util:
	@$(MAKE) -C util -f util.mk util

.PHONY: build-modload
build-modload:
	$(MAKE) -C modload modload
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
	(cd build/kmod-29; ./configure CC=$(abspath build/musl/bin/musl-gcc) CFLAGS="-Os -I $(abspath build/musl/include)" \
	--enable-static --prefix=$(abspath build/kmod) --disable-tools --with-zstd)
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