#include <minlinux/syscall.h>
#include <minlinux/util.h>
#include <minlinux/uring_ctl.h>
#include "include/command.h"

#define CMD_PROTO(name) void name(uring_queue * uring, int argc, \
	char * argv[argc], char * envp[], char * next)

#if ARG_MAX <= 32
#define MAX_ARGS ARG_MAX
#else
#define MAX_ARGS 32
#endif

void dirlist(uring_queue *, const char *);
char * find_arg(char *);
char * terminate_arg(char *);
bool streq(const char *, const char *);
void busybox(char *, char *);
void fill_args(size_t maxargs, char *args[maxargs], char *);
void print_time(void);

CMD_PROTO(cmd_time);
CMD_PROTO(cmd_ls);
CMD_PROTO(cmd_cat);
CMD_PROTO(cmd_memrun);
CMD_PROTO(cmd_env);
CMD_PROTO(cmd_exit);
CMD_PROTO(cmd_usage);

struct {
	char * name;
	void (*func)(uring_queue *uring, int argc, 
		char * argv[argc], char * envp[], char * next);
} builtins[] = {
	{"time", cmd_time},
	{"ls", cmd_ls},
	{"cat", cmd_cat},
	{"env", cmd_env},
	{"exit", cmd_exit},
	{"usage", cmd_usage},
	{"memrun", cmd_memrun}
};

void process_command(char * args, uring_queue * uring, int argc, 
	char * argv[argc], char * envp[]) {
	char * prog = find_arg(args);

	if(prog == NULL) {
		return;
	}

	char * next = terminate_arg(prog);

	for(size_t i = 0; i < ARRAY_LENGTH(builtins); i++) {
		if(streq(prog, builtins[i].name)) {
			builtins[i].func(uring, argc, argv, envp, next);
			return;
		}
	}

	busybox(prog, next);
}

CMD_PROTO(cmd_time) {
	struct timespec time;
	SYSCHECK(clock_gettime(CLOCK_REALTIME, &time));

	WRITESTR("Current Time: ");
	write_int(time.tv_sec);
	WRITESTR(" sec ");
	write_int(time.tv_nsec);
	WRITESTR(" nsec\n");
}

CMD_PROTO(cmd_ls) {
	char * dir = find_arg(next);
	if(dir == NULL) {
		dirlist(uring, "/");
	} else {
		char * next = terminate_arg(dir);
		dirlist(uring, dir);
	}
}

CMD_PROTO(cmd_cat) {
	char * path = find_arg(next);
	if(path == NULL) {
		WRITESTR("No file path specified.\n");
	} else {
		char * next = terminate_arg(path);
		struct open_how how = {
			.flags = O_RDONLY | O_NOATIME
		};
		int fd = openat2(AT_FDCWD, path, &how, sizeof(struct open_how));
		if(fd < 0) {
			WRITE_ERR("Couldn't open file", fd);
			return;
		}
		char buffer[1024];
		int result;
		while((result = read(fd, buffer, sizeof buffer)) > 0) {
			int wrresult;
			if((wrresult = write(STDOUT, buffer, result)) < 0) {
				WRITE_ERR("Error writing to stdout", wrresult);
			}
		}

		if(result < 0) {
			WRITE_ERR("Error reading file", result);
		}
	}
}

CMD_PROTO(cmd_env) {
	for(size_t i = 0; envp[i] != NULL; i++) {
		WRITESTR(envp[i]);
		WRITESTR("\n");
	}
}

CMD_PROTO(cmd_exit) {
	reboot_hard(LINUX_REBOOT_CMD_POWER_OFF);
}

CMD_PROTO(cmd_usage) {
	struct sysinfo info;
	SYSCHECK(sysinfo(&info));
	WRITESTR("Memory Used/Total: ");
	write_int((info.totalram - info.freeram) * info.mem_unit);
	WRITESTR("/");
	write_int(info.totalram * info.mem_unit);
	WRITESTR(" bytes\n");
}

CMD_PROTO(cmd_memrun) {
	char *path = find_arg(next);
	if (path == NULL) {
		WRITESTR("No file path specified.\n");
	} else {
		char *next = terminate_arg(path);
		struct clone_args cl_args = {
			.flags = 0,
			.exit_signal = SIGCHLD
		};
		pid_t tid = clone3(&cl_args, sizeof(struct clone_args));
		if (tid > 0) {
			WRITESTR("Forked process ");
			write_int(tid);
			WRITESTR("\n");

			siginfo_t siginfo;
			SYSCHECK(waitid(P_PID, tid, &siginfo, WEXITED, NULL));

			WRITESTR("Child process ");
			write_int(siginfo.si_pid);
			WRITESTR(" exited with status ");
			write_int(siginfo.si_status);
			WRITESTR("\n");
		} else if (tid == 0) {
			char *args[MAX_ARGS];

			/* Fill in args */
			args[0] = (char *)path;
			fill_args(MAX_ARGS - 1, &args[1], next);

			/* Empty environment */
			char *envp[] = {
				NULL
			};

			close_all_files();

			int stdin = SYSCHECK(open("/dev/console", O_RDONLY, 0));
			int stdout = SYSCHECK(open("/dev/console", O_APPEND | O_WRONLY, 0));
			SYSCHECK(dup3(stdout, 2, 0));

			int memfd = memfd_create(0);
			struct open_how how = {
				.flags = O_CLOEXEC
			};

			int infd = SYSCHECK(openat2(AT_FDCWD, path, &how, sizeof(struct open_how)));

			struct stat stats;

			SYSCHECK(fstatat(infd, "", &stats, AT_EMPTY_PATH));

			off64_t in = 0, out = 0;

			SYSCHECK(copy_file_range(infd, &in, memfd, &out, stats.st_size, 0));

			//exec(memfd, args, envp);

			// https://tejom.github.io/c/linux/containers/docker/2016/10/04/containers-from-scratch-pt1.html
			// https://news.ycombinator.com/item?id=23167383
			// https://www.kernel.org/doc/Documentation/filesystems/ramfs-rootfs-initramfs.txt
			// chroot/pivot_root?
			static sigset_t nosignals = (sigset_t)0;
			SYSCHECK(sigprocmask(SIG_SETMASK, &nosignals, NULL));
			int execres = execveat(memfd, "", args, envp, AT_EMPTY_PATH);
			WRITE_ERR("execveat returned ", execres);
		} else {
			WRITE_ERR("clone3 error", tid);
		}
	}
}

void busybox(char *command, char *next) {
	next = find_arg(next);

	struct clone_args cl_args = {
		.flags = 0,
		.exit_signal = SIGCHLD
	};
	pid_t tid = clone3(&cl_args, sizeof(struct clone_args));
	if (tid > 0) {
		siginfo_t siginfo;
		SYSCHECK(waitid(P_PID, tid, &siginfo, WEXITED, NULL));
		WRITESTR("Busybox exited with status ");
		write_int(siginfo.si_status);
		WRITESTR("\n");
	} else if (tid == 0) {

		char *args[MAX_ARGS];

		/* Fill in args */
		args[0] = "/busybox";
		args[1] = command;
		fill_args(MAX_ARGS - 2, &args[2], next);

		/* Empty environment */
		char *envp[] = {
			NULL
		};

		/* Close all file descriptors, we don't need any of them! */
		close_all_files();

		int stdin = SYSCHECK(open("/dev/console", O_RDONLY, 0));
		int stdout = SYSCHECK(open("/dev/console", O_APPEND | O_WRONLY, 0));
		SYSCHECK(dup3(stdout, 2, 0));

		// https://tejom.github.io/c/linux/containers/docker/2016/10/04/containers-from-scratch-pt1.html
		// https://news.ycombinator.com/item?id=23167383
		// https://www.kernel.org/doc/Documentation/filesystems/ramfs-rootfs-initramfs.txt
		// chroot/pivot_root?

		static sigset_t nosignals = (sigset_t)0;

		SYSCHECK(sigprocmask(SIG_SETMASK, &nosignals, NULL));

		int execres = execveat(AT_FDCWD, "/busybox", args, envp, 0);
		WRITE_ERR("execveat returned ", execres);
	} else {
		WRITE_ERR("clone3 error", tid);
	}
}

void fill_args(size_t maxargs, char *args[maxargs], char * next) {
	size_t i = 0;
	while (next != NULL && i < maxargs) {
		char *temp = terminate_arg(next);
		args[i] = next;
		next = find_arg(temp);
		i++;
	}

	if (i == MAX_ARGS) {
		WRITESTR("Truncated to ");
		write_int(MAX_ARGS);
		WRITESTR(" args\n");
	} else {
		args[i] = NULL;
	}
}

// void exec(int file, char ** args, char *envp[]) {
// 	// SYSCHECK(setuid(1));
// 	//__u32 caps = CAP_TO_MASK();
// 	// setcap(caps, caps, caps);

// 	int stdin = SYSCHECK(open("/dev/console", O_RDONLY, 0));
// 	int stdout = SYSCHECK(open("/dev/console", O_APPEND | O_WRONLY, 0));
// 	SYSCHECK(dup3(stdout, 2, 0));

// 	// https://tejom.github.io/c/linux/containers/docker/2016/10/04/containers-from-scratch-pt1.html
// 	// https://news.ycombinator.com/item?id=23167383
// 	// https://www.kernel.org/doc/Documentation/filesystems/ramfs-rootfs-initramfs.txt
// 	// chroot/pivot_root?

// 	static sigset_t nosignals = (sigset_t)0;

// 	SYSCHECK(sigprocmask(SIG_SETMASK, &nosignals, NULL));

// 	int execres = execveat(file, "", args, envp, AT_EMPTY_PATH);
// 	WRITE_ERR("execveat returned ", execres);
// }

int pathfd(const char * path) {
	struct open_how how = {
		.flags = O_PATH | O_CLOEXEC
	};
	return SYSCHECK(openat2(AT_FDCWD, path, &how, sizeof(struct open_how)));
}

void dirlist(uring_queue *uring, const char * path) {
	/* TODO use uring openat2 */
	int rootdir = open(path, O_RDONLY | O_NOATIME | O_DIRECTORY, 0);
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
			write(STDOUT, entry->d_name, strlen(entry->d_name));
			switch(entry->d_type) {
				case DT_DIR:
					WRITESTR("/\n");
					break;
				case DT_REG:
					WRITESTR("\n");
					break;
				case DT_FIFO:
					WRITESTR(" (fifo)\n");
					break;
				case DT_CHR:
					WRITESTR(" (character)\n");
					break;
				case DT_BLK:
					WRITESTR(" (block)\n");
					break;
				case DT_LNK:
					WRITESTR(" -> ???\n");
					break;
				case DT_SOCK:
					WRITESTR(" (socket)\n");
					break;
				case DT_UNKNOWN:
				default:
					WRITESTR("?\n");
					break;
			}
			pos += entry->d_reclen;
		}
	} while(count > 0);
}

/* Skip blanks to find the next arg, or NULL */
char * find_arg(char * str) {
	while((*str == ' ') || (*str == '\t')) {
		str++;
	}
	if(*str == '\0') {
		return NULL;
	}
	return str;
}

/* Returns the rest of the string after thlse current arg, or NULL */
char * terminate_arg(char * str) {
	while((*str != ' ') && (*str != '\t') && (*str != '\0')) {
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