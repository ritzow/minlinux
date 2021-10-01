#Automatically passed to sub-makes
MAKEFLAGS += --no-print-directory --no-builtin-rules --no-builtin-variables

copy-config = cp $(LINUX_SRC_DIR)/.config $(1)/$(2)-$(shell date --iso-8601=seconds).config

.PHONY: help
help:
	@echo "build: Build the kernel and modules using .config in" $(LINUX_SRC_DIR)
	@echo "download: Download the linux sources into directory" $(LINUX_SRC_DIR)

include projects.mk kernel.mk

.DEFAULT:
	$(error Unsupported target "$@")
