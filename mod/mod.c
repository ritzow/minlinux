#include <libkmod.h>
#include <minlinux.h>

#include "../init/syscall.h"

asm(
	".section .text\n"
	".global _start\n"
	"_start:\n"
	"pop %rdi\n"                // argc   (first arg, %rdi)
	"mov %rsp, %rsi\n"          // argv[] (second arg, %rsi)
	"lea 8(%rsi,%rdi,8),%rdx\n" // then a NULL then envp (third arg, %rdx)
	"and $-16, %rsp\n"          // x86 ABI : esp must be 16-byte aligned when
	"sub $8, %rsp\n"            // entering the callee
	"call main\n"               // main() returns the status code, we'll exit with it.
	"movzb %al, %rdi\n"         // retrieve exit code from 8 lower bits
	"mov $60, %rax\n"           // NR_exit == 60
	"syscall\n"                 // really exit
	"hlt\n"                     // ensure it does not return
);

size_t strlen(const char *);

int main(int argc, char * argv[], char * envp[]) {
	write(1, "Hello ", strlen("Hello "));
	for(int i = 0; i < argc; i++) {
		write(1, argv[i], strlen(argv[i]));
		write(1, " ", strlen(" "));
	}
	write(1, "\n", strlen("\n"));
	return 0;
}

size_t strlen(const char *str) {
	size_t len;
	for (len = 0; str[len]; len++);
	return len;
}