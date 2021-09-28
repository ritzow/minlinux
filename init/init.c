
#include "syscall.h"
#include "uring_ctl.h"
#include "util.h"
#include "command.h"

/* for struct iovec */
#include <linux/uio.h>

void uring_submit_read(uring_queue *, int, void *, size_t);
void uring_submit_write_linked(uring_queue *, int, void *, size_t);

void init(uring_queue *);
void handle(uring_queue *, struct io_uring_cqe *, int argc, char * argv[argc], char * envp[]);
void process_input(uring_queue *, struct io_uring_cqe *, int argc, char * argv[argc], 
	char * envp[]);
void submit_prompt(uring_queue *);
void submit_sig_listen(uring_queue *);
void process_signal(void);

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
	"call start\n"               // main() returns the status code, we'll exit with it.
//	"movzb %al, %rdi\n"         // retrieve exit code from 8 lower bits
//	"mov $60, %rax\n"           // NR_exit == 60
//	"syscall\n"                 // really exit
//	"hlt\n"                     // ensure it does not return
);

static sigset_t allsignals = ~((sigset_t)0); /* no signals currntly */

typedef enum {
	EVENT_CONSOLE_READ,
	//EVENT_WRITE,
	EVENT_OTHER,
	EVENT_SIGNAL,
} io_event;

__attribute__((noreturn))
void start(int argc, char * argv[], char * envp[]) {
	int con = SYSCHECK(open("/dev/console", O_APPEND | O_RDWR, 0));
	SYSCHECK(dup3(con, 1, 0));
	SYSCHECK(dup3(con, 2, 0));

	/*struct timespec time;
	SYSCHECK(clock_gettime(CLOCK_BOOTTIME, &time));

	WRITESTR("Boot to init took ");
	write_int(time.tv_sec);
	WRITESTR(" sec ");
	write_int(time.tv_nsec);
	WRITESTR(" nsec\n");*/

	SYSCHECK(mount(NULL, "/proc", "proc", MS_NOEXEC | MS_RDONLY, NULL));
	SYSCHECK(mount(NULL, "/sys", "sysfs", MS_NOEXEC | MS_RDONLY, NULL));

	uring_queue uring = uring_init(16);

	SYSCHECK(sigprocmask(SIG_SETMASK, &allsignals, NULL, sizeof(sigset_t)));

	init(&uring);

	while(true) {
		uring_wait(&uring, 1);
		struct io_uring_cqe * cqe = uring_result(&uring);
		if(cqe != NULL) {
			handle(&uring, cqe, argc, argv, envp);
			uring_advance(&uring);
		}
	}

	//uring_close(&uring);
}

char buffer[1024];

struct signalfd_siginfo siginfo;

int sigfd;

void init(uring_queue *uring) {
	sigfd = SYSCHECK(signalfd(-1, &allsignals, 0));
	submit_prompt(uring);
	submit_sig_listen(uring);
}

void handle(uring_queue * uring, struct io_uring_cqe * cqe, int argc, 
	char * argv[argc], char * envp[]) {
	switch(cqe->user_data) {
		case EVENT_CONSOLE_READ:
			process_input(uring, cqe, argc, argv, envp);
			break;
		case EVENT_SIGNAL:
			process_signal();
			submit_sig_listen(uring);
			break;
		case EVENT_OTHER:
			SYSCHECK(cqe->res);
			break;
	}
}

void process_signal() {
	switch(siginfo.ssi_signo) {
		case SIGCHLD: {
			/* See sigaction(2) for siginfo field meanings for SIGCHLD */
			siginfo_t info;
			SYSCHECK(waitid(P_PID, siginfo.ssi_pid, &info, WEXITED | WNOHANG, NULL));
			WRITESTR("Child process ");
			write_int(siginfo.ssi_pid);
			WRITESTR(" exited with status ");
			write_int(siginfo.ssi_status);
			WRITESTR("\n");
			break;
		}
		default:
			WRITESTR("Received signal number ");
			write_int(siginfo.ssi_signo);
			WRITESTR("\n");
			break;
	}
}

void process_input(uring_queue *uring, struct io_uring_cqe *cqe, 
	int argc, char * argv[argc], char * envp[]) {
	if (cqe->res > 0) {
		/* TODO support reading less than a full line with newline char */
		size_t last_index = cqe->res - 1; 
		if(buffer[last_index] != '\n') {
			WRITESTR("No end of line.\n");
		} else {
			buffer[last_index] = '\0';
			process_command(buffer, uring, argc, argv, envp);
		}
		submit_prompt(uring);
	} else if(cqe->res == 0) {
		/* reached EOF */
		ERREXIT("End of console input.\n");
	} else if(cqe->res < 0) {
		ERREXIT("Error reading file.\n");
	}
}

void submit_sig_listen(uring_queue *uring) {
	struct io_uring_sqe * sqe = uring_new_submission(uring);
	sqe->opcode = IORING_OP_READ;
	sqe->fd = sigfd;
	sqe->addr = (uintptr_t)&siginfo;
	sqe->len = sizeof(struct signalfd_siginfo);
	sqe->flags = 0;
	sqe->ioprio = 0;
	sqe->user_data = EVENT_SIGNAL;
}

void submit_prompt(uring_queue *uring) {
	uring_submit_write_linked(uring, 1, "Command: ", strlen("Command: "));
	uring_submit_read(uring, 1, buffer, sizeof buffer);
}

void uring_submit_read(uring_queue * uring, int fd, void * dst, size_t count) {
	struct io_uring_sqe * sqe = uring_new_submission(uring);
	sqe->opcode = IORING_OP_READ;
	sqe->fd = fd;
	sqe->addr = (uintptr_t)dst;
	sqe->len = count;
	sqe->flags = 0;
	sqe->ioprio = 0;
	sqe->user_data = EVENT_CONSOLE_READ;
}

void uring_submit_write_linked(uring_queue * uring, int fd, void * src, size_t count) {
	struct io_uring_sqe * sqe = uring_new_submission(uring);
	sqe->opcode = IORING_OP_WRITE;
	sqe->fd = fd;
	sqe->addr = (uintptr_t)src;
	sqe->len = count;
	sqe->flags = IOSQE_IO_LINK;
	sqe->ioprio = 0;
	sqe->user_data = EVENT_OTHER;
}
