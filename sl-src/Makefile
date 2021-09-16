CC=gcc
CFLAGS=-Wall -g -Werror -Wno-unused-variable -fno-asynchronous-unwind-tables \
	-fno-ident -Os -nostdlib -static -lgcc -I $(LINUX_INCLUDES) \
	-march=x86-64
#-march=x86-64-v3

SHARED_HEADERS=syscall.h util.h

init: uring_ctl.o main.c uring_ctl.h util.o $(SHARED_HEADERS)
	$(CC) $(CFLAGS) -s main.c uring_ctl.o util.o -o init

uring_ctl.o: uring_ctl.c uring_ctl.h $(SHARED_HEADERS)
	$(CC) $(CFLAGS) -c uring_ctl.c -o uring_ctl.o

util.o: util.c $(SHARED_HEADERS)
	$(CC) $(CFLAGS) -c util.c -o util.o

.PHONY: clean
clean:
	rm uring_ctl.o util.o init