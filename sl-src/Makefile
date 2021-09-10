
.PHONY: init
init:
	gcc -Wall -Wextra -march=x86-64-v3 -O3 -masm=intel -pie -static \
		-nolibc -nostdlib -nodefaultlibs -nostdinc -I $(LINUX_SRC_DIR)/include \
		main.c syscall.c syscall.h -o init
#		/usr/lib/klibc/lib/libc.so -I /usr/lib/klibc/include -I /usr/lib/klibc/include/bits64 \
#		-I /usr/lib/klibc/include/arch/x86_64 \
#		-D __KLIBC__ -D _BITSIZE=64 \
		
