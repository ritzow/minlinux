#include <asm/unistd.h>

#include "syscall.h"

/* From linux tools/include/nolibc/nolibc.h */

static inline long syscall0(long num) {
	long _ret;
	register long _num  asm("rax") = (num);
	
	asm volatile (
		"syscall\n"
		: "=a" (_ret)
		: "0"(_num)
		: "rcx", "r8", "r9", "r10", "r11", "memory", "cc"
	);
	return _ret;
}

pid_t getpid() {
	return syscall0(__NR_getpid);
}

//int open(const char * path, int flags, mode_t mode)
