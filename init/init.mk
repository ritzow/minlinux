include $(COMMON_PRE_MK)

CC := $(CC_STATIC)
CFLAGS := $(CFLAGS_STATIC)

project := init
programs := init

project-deps := util

init-deps := init/command.o init/init.o util/uring_ctl.o util/util.o

command.o-deps := command.c
command.c-deps := include/command.h

init.o-deps := init.c
init.c-deps := $(INCLUDE_DIR)/minlinux/*.h

include $(COMMON_POST_MK)