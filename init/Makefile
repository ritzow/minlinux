CC=gcc
CFLAGS=-Wall -Wextra -g -Werror \
	-Wno-unused-parameter -Wno-unused-variable \
	-fno-asynchronous-unwind-tables \
	-fno-ident -Os -nostdlib -static -lgcc \
	-I $(BUILD_DIR)/include \
	-march=x86-64
#-march=x86-64-v3

SHARED_HEADERS=syscall.h util.h

init: uring_ctl.o init.c uring_ctl.h util.o command.o $(SHARED_HEADERS) command.h
	$(CC) $(CFLAGS) -s init.c uring_ctl.o util.o command.o $(BUILD_DIR)/jemalloc-5.2.1/lib/libjemalloc_pic.a  -o init

uring_ctl.o: uring_ctl.c uring_ctl.h $(SHARED_HEADERS)
	$(CC) $(CFLAGS) -c uring_ctl.c -o uring_ctl.o

util.o: util.c $(SHARED_HEADERS)
	$(CC) $(CFLAGS) -c util.c -o util.o

command.o: command.c $(SHARED_HEADERS) command.h
	$(CC) $(CFLAGS) -c command.c -o command.o

.PHONY: clean
clean:
	rm *.o init