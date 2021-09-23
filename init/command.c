#include "syscall.h"
#include "util.h"
#include "uring_ctl.h"
#include "command.h"

void dirlist(uring_queue *, const char *);
char * find_arg(char *);
char * terminate_arg(char *);
bool streq(const char *, const char *);

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
			switch(entry->d_type) {
				case DT_DIR:
					WRITESTR("/\n");
					break;
				case DT_REG:
					WRITESTR("\n");
					break;
				case DT_FIFO:
				case DT_CHR:
				case DT_BLK:
				case DT_LNK:
				case DT_SOCK:
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