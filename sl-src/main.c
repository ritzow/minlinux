
#include "syscall.h"
#include "uring_ctl.h"
#include "util.h"

/* for struct iovec */
#include <linux/uio.h>

void dirlist(const char *);
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
	int con = SYSCHECK(open("/dev/console", O_APPEND | O_RDWR, 0));
	/* Setup io_uring for use */

	uring_queue uring = uring_init(16);

	char buffer[1024];

//     uintptr_t cur = (uintptr_t)brk(0);
//     write_int(cur);

//     //write_int((uintptr_t)brk((void*)(cur + 100)));

//    // write_int((uintptr_t)brk(0));

//     //write_int((uintptr_t)brk((void*)(brk(0) - 100)));

	// void * addr = (void*)SYSCHECK(mmap(brk(0), PAGE_SIZE, PROT_READ | PROT_WRITE, 
	//     MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0));

//     write_int(pos);

	// struct iovec buffer = {
	// 	.iov_base 
	// };

	//SYSCHECK(io_uring_register(uring.ringfd, IORING_RGISTER_BUFFERS, &buffer, 1));

	SYSCHECK(mount(NULL, "/proc", "proc", MS_NOEXEC | MS_RDONLY, NULL));

	SYSCHECK(mount(NULL, "/sys", "sysfs", MS_NOEXEC | MS_RDONLY, NULL));

	struct timespec time;
	SYSCHECK(clock_gettime(CLOCK_MONOTONIC, &time));

	write_int(time.tv_sec);
	WRITESTR(" sec ");
	write_int(time.tv_nsec);
	WRITESTR(" nsec\n");

	while (1) {
		//uring_submit_write(&uring, con, "Command: ", strlen("Command: "));

		/* linked write */
		struct io_uring_sqe * sqe = uring_new_submission(&uring);
		sqe->opcode = IORING_OP_WRITE;
		sqe->fd = con;
		sqe->addr = (uintptr_t)"Command: ";
		sqe->len = strlen("Command: ");
		sqe->flags = IOSQE_IO_LINK;
		sqe->ioprio = 0;
		
		uring_submit_read(&uring, con, buffer, sizeof buffer);
		/* todo uring_submit return number of new entries and iterate over */
		memset(buffer, 0, sizeof buffer);
		uring_wait(&uring, 2);
		read_from_cq(&uring);
		int res = read_from_cq(&uring);
		if (res > 0) {
			uring_submit_write(&uring, con, buffer, strlen(buffer));
			uring_wait(&uring, 1);
			read_from_cq(&uring);
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

void dirlist(const char * path) {
	int rootdir = SYSCHECK(open(path, O_RDONLY | O_DIRECTORY, 0));
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

