#include "syscall.h"
#include "util.h"
#include "uring_ctl.h"
#include "command.h"

void dirlist(uring_queue *, const char *);
char * find_arg(char *);
char * terminate_arg(char *);
bool streq(const char *, const char *);
void cat(uring_queue *, const char * );
void run(uring_queue *, const char *, char *);
void exec_prog(const char *, char *);

void process_command(char * args, uring_queue * uring, int argc, 
	char * argv[argc], char * envp[]) {
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
		char * path = find_arg(next);
		if(path == NULL) {
			WRITESTR("No file path specified.\n");
		} else {
			char * next = terminate_arg(path);
			cat(uring, path);
		}
	} else if(streq(prog, "env")) {
		for(size_t i = 0; envp[i] != NULL; i++) {
			WRITESTR(envp[i]);
			WRITESTR("\n");
		}
	} else if(streq(prog, "run")) {
		char * path = find_arg(next);
		if(path == NULL) {
			WRITESTR("No file path specified.\n");
		} else {
			char * next = terminate_arg(path);
			run(uring, path, next);
		}
	} else if(streq(prog, "exit")) {
		reboot_hard(LINUX_REBOOT_CMD_POWER_OFF);
	} else {
		WRITESTR("Unknown command \"");
		WRITESTR(prog);
		WRITESTR("\"\n");
	}
}

void run(uring_queue *uring, const char * path, char * next) {
	struct clone_args cl_args = {
		.flags = CLONE_NEWNET | CLONE_NEWIPC | CLONE_NEWCGROUP |
		 	CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUSER | CLONE_NEWUTS |
			CLONE_CLEAR_SIGHAND,
		.exit_signal = SIGCHLD
	};

	pid_t tid = clone3(&cl_args, sizeof(struct clone_args));

	uint32_t mutex = 0;

	if(tid > 0) {
		WRITESTR("Forked process ");
		write_int(tid);
		WRITESTR("\n");
	} else if(tid == 0) {
		exec_prog(path, next);
	} else {
		WRITE_ERR("clone3 error", tid);
	}
}

void exec_prog(const char * path, char * next) {
	//use memfd_create to load file into memory first and exec from memory?
	struct open_how how = {
		.flags = O_PATH | O_CLOEXEC
	};

	int fd = SYSCHECK(openat2(AT_FDCWD, path, &how, sizeof(struct open_how)));

	int stdout = SYSCHECK(open("/dev/console", O_APPEND | O_RDWR, 0));

#if ARG_MAX <= 32
#define MAX_ARGS ARG_MAX
#else
#define MAX_ARGS 32
#endif

	char * args[MAX_ARGS];

	/* Fill in args */
	args[0] = (char *)path;
	size_t i = 1;
	while(next != NULL && i < MAX_ARGS) {
		char * temp = terminate_arg(next);
		args[i] = next;
		next = find_arg(temp);
		i++;
	}
	if(i == MAX_ARGS) {
		WRITESTR("Truncated to ");
		write_int(MAX_ARGS);
		WRITESTR(" args\n");
	} else {
		args[i] = NULL;
	}

	/* Empty environment */
	char * envp[] = {
		NULL
	};

	int execres = execveat(fd, "", args, envp, AT_EMPTY_PATH);
	WRITE_ERR("execveat returned ", execres);
}

void cat(uring_queue *uring, const char * path) {
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
		if((wrresult = write(0, buffer, result)) < 0) {
			WRITE_ERR("Error writing to stdout", wrresult);
		}
	}

	if(result < 0) {
		WRITE_ERR("Error reading file", result);
	}
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
			write(0, entry->d_name, strlen(entry->d_name));
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