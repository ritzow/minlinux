
#include "syscall.h"
#include "uring_ctl.h"
#include "util.h"
#include "command.h"

/* for struct iovec */
#include <linux/uio.h>

void dirlist(uring_queue *, const char *);
bool streq(const char *, const char *);
char * find_arg(char *);
char * terminate_arg(char *);
void process_command(char *, uring_queue *, char * envp[]);

/* startup code from nolibc.h */
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

int main(int argc, char * argv[], char * envp[]) {
	int con = SYSCHECK(open("/dev/console", O_APPEND | O_RDWR, 0));

	struct timespec time;
	SYSCHECK(clock_gettime(CLOCK_BOOTTIME, &time));

	WRITESTR("Boot to init took ");
	write_int(time.tv_sec);
	WRITESTR(" sec ");
	write_int(time.tv_nsec);
	WRITESTR(" nsec\n");

	SYSCHECK(mount(NULL, "/proc", "proc", MS_NOEXEC | MS_RDONLY, NULL));
	SYSCHECK(mount(NULL, "/sys", "sysfs", MS_NOEXEC | MS_RDONLY, NULL));

	uring_queue uring = uring_init(16);
	char buffer[1024];

	while (1) {
		uring_submit_write_linked(&uring, con, "Command: ", strlen("Command: "));
		uring_submit_read(&uring, con, buffer, sizeof buffer);
		/* todo uring_submit return number of new entries and iterate over */
		memset(buffer, 0, sizeof buffer);
		uring_wait(&uring, 2);
		SYSCHECK(uring_result(&uring).entry.res);
		int res = uring_result(&uring).entry.res;
		if (res > 0) {
			/* TODO support reading less than a full line with newline char */
			if(buffer[res - 1] != '\n') {
				uring_submit_write_linked(&uring, con, "Line too long\n", 
					strlen("line too long\n"));
				uring_wait(&uring, 1);
				SYSCHECK(uring_result(&uring).entry.res);
				/* TODO read until newline */
				continue;
			}

			buffer[res - 1] = '\0';
			process_command(buffer, &uring, envp);
		} else if (res == 0) {
			/* reached EOF */
			SYSCHECK(write(con, "End of stdin\n", strlen("End of stdin\n")));
			break;
		} else if (res < 0) {
			ERREXIT("Error reading file");
		}
	}

	uring_close(&uring);
}
