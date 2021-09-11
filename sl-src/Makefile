
.PHONY: init
init:
	# gcc -Wall -Wextra -march=x86-64-v3 -O3 -masm=intel -pie -static \
	# 	-nolibc -nostdlib -nodefaultlibs -nostdinc -I $(LINUX_INCLUDES) \
	# 	main.c -o init #syscall.c syscall.h 
	
	gcc -fno-asynchronous-unwind-tables -fno-ident -s -Os -nostdlib -I $(LINUX_INCLUDES) \
		-static -o init main.c -lgcc -march=x86-64-v3