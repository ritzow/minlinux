
#include "syscall.h"

#define CHECK(expr) ({\
	int res = (expr);\
	if(res < 0) {\
		sys_exit(res);\
	}\
	res;\
})


/* TODDO mremap mprotect io_uring stuff */

bool streq(const char *, const char *);

int main(int argc, char * argv[], char * envp[]) {
	int con = sys_open("/dev/console", O_APPEND | O_RDWR, 0);
	int rootdir = sys_open("/", O_RDONLY | O_DIRECTORY, 0);
	int64_t count;
	do {
		uint8_t buffer[1024];
		count = sys_getdents64(rootdir, (struct linux_dirent64*)&buffer, 1024);
		for(int64_t pos = 0; pos < count;) {
			struct linux_dirent64 * entry = (struct linux_dirent64*)(buffer + pos);
			sys_write(con, entry->d_name, strlen(entry->d_name));
			sys_write(con, "\n", 1);
			pos += entry->d_reclen;
		}
	} while(count > 0);

	char buffer[64];

	while(1) {
		ssize_t count = sys_read(con, buffer, sizeof buffer);
		*strchr(buffer, '\n') = '\0';
		if(streq(buffer, "exit")) {
			sys_reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_POWER_OFF, 0);
		} else {
			sys_write(con, "Unknown command \"", sizeof "Unknown command \"");
			sys_write(con, buffer, count - 1);
			sys_write(con, "\"\n", sizeof "\"\n");
			//const char * str = ltoa((long)&main);
			//sys_write(con, str, strlen(str));
		}
	}
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

