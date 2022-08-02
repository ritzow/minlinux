
#include <minlinux/syscall.h>
#include <minlinux/uring_ctl.h>
#include <minlinux/util.h>
#include "include/command.h"

/* For mode_t creation */
#include <linux/stat.h>

void init(uring_queue *);
void handle(uring_queue *, struct io_uring_cqe *, int argc, char * argv[argc], char * envp[]);
void process_input(uring_queue *, struct io_uring_cqe *, int argc, char * argv[argc], 
	char * envp[]);
void create_console_prompt(uring_queue *);
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
);

//TODO why is this 1 instead of 0?
static sigset_t allsignals = ~((sigset_t)0);

enum {
	EVENT_CONSOLE_READ,
	EVENT_SIGNAL,
	EVENT_OTHER
};

__attribute__((noreturn))
void start(int argc, char * argv[], char * envp[]) {

	umask(0);

	/* These permissions will be overwritten by the mounted file systems */
	SYSCHECK(mkdir("/proc", S_IRWXU | S_IRWXG | S_IRWXO));
	SYSCHECK(mkdir("/dev", S_IRWXU | S_IRWXG | S_IRWXO));
	SYSCHECK(mkdir("/sys", S_IRWXU | S_IRWXG | S_IRWXO));

	SYSCHECK(mount(NULL, "/proc", "proc", MS_NOEXEC, NULL));
	SYSCHECK(mount(NULL, "/sys", "sysfs", MS_NOEXEC | MS_RDONLY, NULL));
	SYSCHECK(mount(NULL, "/dev", "devtmpfs", MS_NOEXEC, NULL));

	// specific serial port is at /sys/devices/platform/serial8250/tty
	int con = SYSCHECK(open("/dev/console", O_RDONLY, 0));
	int stdout = SYSCHECK(open("/dev/console", O_APPEND | O_WRONLY, 0));
	SYSCHECK(dup3(stdout, 2, 0));

	int printkfd = SYSCHECK(open("/proc/sys/kernel/printk", O_WRONLY, 0));
	SYSCHECK(write(printkfd, "0", 1));
	SYSCHECK(close(printkfd));

	uring_queue uring = uring_init(16);

	/* Ignore all signals because they are handled by signalfd */
	SYSCHECK(sigprocmask(SIG_SETMASK, &allsignals, NULL));

	init(&uring);

	char uplo[] = "ip link set lo up";
	char upeth[] = "ip link set eth0 up";
	char dhcp[] = "udhcpc -f -n -q";
	char addrset[] = "ip a add 10.0.2.15 dev eth0";
	char runsh[] = "sh";

	SYSCHECK(socket(AF_PACKET, SOCK_DGRAM, __bswap_16(ETH_P_ALL)));

	//process_command(uplo, &uring, argc, argv, envp);
	//process_command(upeth, &uring, argc, argv, envp);
	//process_command(dhcp, &uring, argc, argv, envp);
	//process_command(addrset, &uring, argc, argv, envp);
	//process_command(runsh, &uring, argc, argv, envp);

	while (true) {
		uring_wait(&uring, 1);
		struct io_uring_cqe * cqe = uring_result(&uring);
		if(cqe != NULL) {
			handle(&uring, cqe, argc, argv, envp);
			uring_advance(&uring);
		}
	}
}

char buffer[1024];

struct signalfd_siginfo siginfo;

int sigfd;

void init(uring_queue *uring) {
	sigfd = SYSCHECK(signalfd(-1, &allsignals, 0));
	create_console_prompt(uring);
	submit_sig_listen(uring);
}

void handle(uring_queue * restrict uring, struct io_uring_cqe * restrict cqe, int argc, 
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
		default:
			WRITESTR("Received signal number ");
			write_int(siginfo.ssi_signo);
			WRITESTR("\n");
			break;
	}
}

void process_input(uring_queue * restrict uring, struct io_uring_cqe * restrict cqe, 
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
		create_console_prompt(uring);
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

void create_console_prompt(uring_queue * uring) {
	struct io_uring_sqe * sqe = uring_new_submission(uring);
	sqe->opcode = IORING_OP_WRITE;
	sqe->fd = STDOUT;
	sqe->addr = (uintptr_t)"Command: ";
	sqe->len = strlen("Command: ");
	sqe->flags = IOSQE_IO_LINK;
	sqe->ioprio = 0;
	sqe->user_data = EVENT_OTHER;

	sqe = uring_new_submission(uring);
	sqe->opcode = IORING_OP_READ;
	sqe->fd = STDIN;
	sqe->addr = (uintptr_t)&buffer;
	sqe->len = sizeof(buffer);
	sqe->flags = 0;
	sqe->ioprio = 0;
	sqe->user_data = EVENT_CONSOLE_READ;
}
