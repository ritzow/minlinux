include $(COMMON_PRE_MK)

CC := $(CC_STATIC)
CFLAGS := $(CFLAGS_STATIC)

project := mod
programs := modload

project-deps := util

modload-deps := mod/mod.o util/util.o
mod.o-deps := mod.c
mod.c-deps := $(INCLUDE_DIR)/minlinux/*.h

include $(COMMON_POST_MK)