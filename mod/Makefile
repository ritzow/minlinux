CC=gcc
CFLAGS=-Wall -g -Werror -Wno-unused-variable -fno-asynchronous-unwind-tables \
	-fno-ident -Os -nostdlib -static -lgcc -I $(BUILD_DIR)/include -I $(BUILD_DIR)/kmod-29/libkmod \
	-march=x86-64

.PHONY: modload
modload:
	$(CC) $(CFLAGS) modload.c -o modload