#include <minlinux/syscall.h>
#include <minlinux/util.h>

int main(int argc, char *argv[], char** env) {
	write(1, "hello world!\n", strlen("hello world!\n"));
	exit(0);
}