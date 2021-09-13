.PHONY: init
init:
	gcc -Wall -Werror -Wno-unused-variable -fno-asynchronous-unwind-tables \
		-fno-ident -s -Os -nostdlib -I $(LINUX_INCLUDES) \
		-static -o init main.c -lgcc -march=x86-64-v3 