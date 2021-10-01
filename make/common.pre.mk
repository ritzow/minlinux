CC_STATIC=gcc
CFLAGS_STATIC=-Wall -Wextra -Werror \
	-Wno-unused-parameter -Wno-unused-variable \
	-fno-asynchronous-unwind-tables \
	-fno-ident -Os -nostdlib -static -lgcc \
	-I $(BUILD_DIR)/include -I $(INCLUDE_DIR) \
	-march=x86-64

BUILD_DIR = $(PROJECT_DIR)/build
OUTPUT_DIR = $(BUILD_DIR)/projects
INCLUDE_DIR = $(PROJECT_DIR)/include

COMMON_POST_MK = $(PROJECT_DIR)/make/common.post.mk