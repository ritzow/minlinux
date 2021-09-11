#include <nolibc/nolibc.h>

int main(int argc, char * argv[], char * envp[]) {
	int con = sys_open("/dev/console", O_APPEND | O_WRONLY, 0);

	while(1) {
		/* Busy wait */
		//pid_t pid = sys_getpid();
		sys_write(1, "hello linux!\n", sizeof "hello linux!\n");
		sleep(1);
	}
}
