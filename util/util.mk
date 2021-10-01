include $(COMMON_PRE_MK)

CC := $(CC_STATIC)
CFLAGS := $(CFLAGS_STATIC)

project := util
objects := util.o uring_ctl.o

util.o-deps := util.c
util.c-deps := $(INCLUDE_DIR)/minlinux/*.h

uring_ctl.o-deps := uring_ctl.c
uring_ctl.c-deps := $(INCLUDE_DIR)/minlinux/*.h

include $(COMMON_POST_MK)