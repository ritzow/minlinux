//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>

#include "syscall.h"

void _start() {
	//int in = open("/dev/console", O_APPEND);
	
	//int fd = open("/dev/console", O_APPEND, 

	while(1) {
		/* Busy wait */
		pid_t pid = getpid();
		//write(in, "hello\n", sizeof "hello\n");
		//sleep(1);
	}
}
