#Automatically passed to sub-makes
MAKEFLAGS += --no-print-directory --no-builtin-rules --no-builtin-variables

#PROJECT_DIR = $(abspath .)
#BUILD_DIR = $(PROJECT_DIR)/build

# .PHONY: help
# help:
# 	@echo $(PROJECT_DIR)/kernel.mk contains kernel build code

# .PHONY: minlinux
# minlinux:
# 	@python3 -B build.py minlinux

#include projects.mk kernel.mk

.PHONY: kernel
kernel:
	@python3 -B build.py $@

.PHONY: %
%:
	@python3 -B build.py $@

#.DEFAULT:
#	@python3 -B build.py $@
#	$(error Unsupported target "$@")
