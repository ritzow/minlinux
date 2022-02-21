COMMON_PRE_MK = $(PROJECT_DIR)/make/common.pre.mk

export PROJECT_DIR COMMON_PRE_MK

.PHONY: %.proj
%.proj:
	@$(MAKE) -C $* -f $*.mk $*