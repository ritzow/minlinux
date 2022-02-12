#ifndef __UTIL_H__
#define __UTIL_H__

#include "syscall.h"

typedef struct {
	const char * str;
	/* large enough for -9223372036854775808 */
	char val[21];
} intstr;

#define strlen(str) ({                          \
	__builtin_constant_p((str)) ?           \
		__builtin_strlen((str)) :       \
		nolibc_strlen((str));           \
})

#define xstr(a) str(a)
#define str(a) #a
#define MARK() write(0, __FILE__ ":" xstr(__LINE__) "\n", \
	__builtin_strlen(__FILE__ ":" xstr(__LINE__) "\n"))

#define WRITESTR(str) ({\
	write(0, (str), strlen(str));\
})

#define SYSCHECK(expr) ({\
	intmax_t res = (expr);\
	if(res < 0) {\
		WRITESTR(__FILE__ ":" xstr(__LINE__) " syscall error ");\
		intstr str = int_to_str(res);\
		write(0, str.str, strlen(str.str));\
		WRITESTR("\n");\
		exit(-res);\
	}\
	res;\
})

#define WRITE_ERR(str, err) ({\
	WRITESTR(str);\
	WRITESTR(": ");\
	write_int(err);\
	WRITESTR("\n");\
})

#define ERREXIT(msg) ({\
	WRITESTR(__FILE__ ":" xstr(__LINE__) " " msg);\
	reboot_hard(LINUX_REBOOT_CMD_POWER_OFF);\
})

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(*a))

void *memmove(void *dst, const void *src, size_t len);
void *memcpy(void *dst, const void *src, size_t len);
void *memset(void *dst, int b, size_t len);
int memcmp(const void *s1, const void *s2, size_t n);
char *strcpy(char *dst, const char *src);
size_t nolibc_strlen(const char *str);
char *strchr(const char *s, int c);
intstr int_to_str(intmax_t in);
const char *ltoa(long in);
void write_int(uint64_t val);

#endif