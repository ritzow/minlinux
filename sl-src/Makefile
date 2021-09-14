CC=gcc
CFLAGS=-Wall -g -Werror -Wno-unused-variable -fno-asynchronous-unwind-tables \
	-fno-ident -Os -nostdlib -static -lgcc -I $(LINUX_INCLUDES) \
	-march=x86-64
#-march=x86-64-v3

init: uring_ctl.o main.c syscall.h uring_ctl.h
	$(CC) $(CFLAGS) main.c uring_ctl.o -o init # -s

uring_ctl.o: uring_ctl.c syscall.h uring_ctl.h
	$(CC) $(CFLAGS) -c uring_ctl.c -o uring_ctl.o