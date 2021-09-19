
#include "syscall.h"
#include "uring_ctl.h"
#include "util.h"

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
	""
);

int main(int argc, char * argv[], char * envp[]) {
	int con = SYSCHECK(open("/dev/console", O_APPEND | O_RDWR, 0));

//     uintptr_t cur = (uintptr_t)brk(0);
//     write_int(cur);

//     //write_int((uintptr_t)brk((void*)(cur + 100)));

//    // write_int((uintptr_t)brk(0));

//     //write_int((uintptr_t)brk((void*)(brk(0) - 100)));

	// void * addr = (void*)SYSCHECK(mmap(brk(0), PAGE_SIZE, PROT_READ | PROT_WRITE, 
	//     MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0));

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

void process_command(char * args, uring_queue * uring, char * envp[]) {
	char * prog = find_arg(args);

	if(prog == NULL) {
		return;
	}

	char * next = terminate_arg(prog);

	if(streq(prog, "ls")) {
		char * dir = find_arg(next);
		if(dir == NULL) {
			dirlist(uring, "/");
		} else {
			char * next = terminate_arg(dir);
			dirlist(uring, dir);
		}
	} else if(streq(prog, "ps")) {
		WRITESTR("ps not implemented\n");
	} else if(streq(prog, "cat")) {
		WRITESTR("cat not implemented\n");
	} else if(streq(prog, "env")) {
		for(size_t i = 0; envp[i] != NULL; i++) {
			WRITESTR(envp[i]);
			WRITESTR("\n");
		}
	} else if(streq(prog, "exit")) {
		reboot_hard(LINUX_REBOOT_CMD_POWER_OFF);
	} else {
		WRITESTR("Unknown command\n");
	}
}

void dirlist(uring_queue *uring, const char * path) {
	/* TODO use uring openat2 */
	int rootdir = open(path, O_RDONLY | O_DIRECTORY, 0);
	if(rootdir < 0) {
		WRITESTR("Error opening directory ");
		WRITESTR(path);
		WRITESTR(": ");
		write_int(rootdir);
		WRITESTR("\n");
		return;
	}
	int64_t count;
	do {
		uint8_t buffer[1024];
		count = getdents64(rootdir, (struct linux_dirent64*)&buffer, sizeof buffer);
		for(int64_t pos = 0; pos < count;) {
			struct linux_dirent64 * entry = (struct linux_dirent64*)(buffer + pos);
			write(0, entry->d_name, strlen(entry->d_name));
			write(0, "\n", 1);
			pos += entry->d_reclen;
		}
	} while(count > 0);
}

/* Skip blanks to find the next arg, or NULL */
char * find_arg(char * str) {
	while(*str == ' '|| *str == '\t') {
		str++;
	}
	if(*str == '\0') {
		return NULL;
	}
	return str;
}

/* Returns the rest of the string after thlse current arg, or NULL */
char * terminate_arg(char * str) {
	while(*str != ' ' && *str != '\t' && *str != '\0') {
		str++;
	}
	if(*str == '\0') {
		return str;
	}
	*str = '\0';
	return str + 1;
}

bool streq(const char * str1, const char * str2) {
	do {
		if(*str1 != *str2) {
			return false;
		}
		if(*str1 == '\0') {
			return true;
		}
		str1++;
		str2++;
	} while(true);
}

