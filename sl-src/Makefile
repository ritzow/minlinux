
.PHONY: init
init:
	gcc -Wall -Wextra -march=x86-64-v3 -O3 -pie -static \
		-nolibc -nostdlib -nodefaultlibs -nostdinc\
		/usr/lib/klibc/lib/libc.so -I /usr/lib/klibc/include -I /usr/lib/klibc/include/bits64\
		-I /usr/lib/klibc/include/arch/x86_64\
		-D __KLIBC__ -D _BITSIZE=64\
		main.c -o init
