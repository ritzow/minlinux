
#include "syscall.h"
#include "uring_ctl.h"

void rootinfo(int);
bool streq(const char *, const char *);

/* TODDO mremap mprotect */

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
	int con = sys_open("/dev/console", O_APPEND | O_RDWR, 0);
    /* Setup io_uring for use */
    uring_queue uring = setup_uring();

	off_t offset = 0;
	char buffer[1024];

    /* 
    * A while loop that reads from stdin and writes to stdout.
    * Breaks on EOF.
    */
    while (1) {
        /* Initiate read from stdin and wait for it to complete */
        submit_to_sq(&uring, con, IORING_OP_READ, sizeof buffer, buffer, offset);
        /* Read completion queue entry */
        int res = read_from_cq(&uring);
        if (res > 0) {
            /* Read successful. Write to stdout. */
            submit_to_sq(&uring, con, IORING_OP_WRITE, sizeof buffer, buffer, offset);
            read_from_cq(&uring);
        } else if (res == 0) {
            /* reached EOF */
            sys_write(con, "End of stdin\n", strlen("End of stdin\n"));
            break;
        } else if (res < 0) {
            /* Error reading file */
			sys_exit(1);
        }
        offset += res;
    }
}

void rootinfo(int out) {
	int rootdir = sys_open("/", O_RDONLY | O_DIRECTORY, 0);
	int64_t count;
	do {
		uint8_t buffer[1024];
		count = sys_getdents64(rootdir, (struct linux_dirent64*)&buffer, sizeof buffer);
		for(int64_t pos = 0; pos < count;) {
			struct linux_dirent64 * entry = (struct linux_dirent64*)(buffer + pos);
			sys_write(out, entry->d_name, strlen(entry->d_name));
			sys_write(out, "\n", 1);
			pos += entry->d_reclen;
		}
	} while(count > 0);
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

