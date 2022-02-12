CC_STATIC=gcc
CFLAGS_STATIC=-Wall -Wextra -Werror \
	-Wno-unused-parameter -Wno-unused-variable \
	-fno-asynchronous-unwind-tables \
	-fno-ident -O3 -nostdlib -static -lgcc \
	-I $(BUILD_DIR)/include -I $(INCLUDE_DIR) \
	-march=icelake-server

BUILD_DIR = $(PROJECT_DIR)/build
OUTPUT_DIR = $(BUILD_DIR)/projects
INCLUDE_DIR = $(PROJECT_DIR)/include

COMMON_POST_MK = $(PROJECT_DIR)/make/common.post.mk

git=git clone --single-branch --depth 1 --branch $(2) $(1) $(BUILD_DIR)/$(3)
