#include "syscall.h"
#include "util.h"

void *memmove(void *dst, const void *src, size_t len) {
	ssize_t pos = (dst <= src) ? -1 : (long)len;
	void *ret = dst;

	while (len--) {
		pos += (dst <= src) ? 1 : -1;
		((char *)dst)[pos] = ((char *)src)[pos];
	}
	return ret;
}

void *memcpy(void *dst, const void *src, size_t len) {
	return memmove(dst, src, len);
}

void *memset(void *dst, int b, size_t len) {
	char *p = dst;

	while (len--)
		*(p++) = b;
	return dst;
}
int memcmp(const void *s1, const void *s2, size_t n) {
	size_t ofs = 0;
	char c1 = 0;

	while (ofs < n && !(c1 = ((char *)s1)[ofs] - ((char *)s2)[ofs])) {
		ofs++;
	}
	return c1;
}
char *strcpy(char *dst, const char *src) {
	char *ret = dst;

	while ((*dst++ = *src++));
	return ret;
}

size_t nolibc_strlen(const char *str) {
	size_t len;

	for (len = 0; str[len]; len++);
	return len;
}
char *strchr(const char *s, int c) {
	while (*s) {
		if (*s == (char)c)
			return (char *)s;
		s++;
	}
	return NULL;
}

intstr int_to_str(intmax_t in) {
	intstr buffer;
	char       *pos = buffer.val + sizeof(buffer.val) - 1;
	bool         neg = in < 0;
	unsigned long n = neg ? -in : in;

	*pos-- = '\0';
	do {
		*pos-- = '0' + n % 10;
		n /= 10;
		if (pos < buffer.val) {
			buffer.str = pos + 1;
			return buffer;
		}
	} while (n);

	if (neg)
		*pos-- = '-';
	buffer.str = pos + 1;
	return buffer;
}

const char *ltoa(long in) {
	/* large enough for -9223372036854775808 */
	static char buffer[21];
	char       *pos = buffer + sizeof(buffer) - 1;
	int         neg = in < 0;
	unsigned long n = neg ? -in : in;

	*pos-- = '\0';
	do {
		*pos-- = '0' + n % 10;
		n /= 10;
		if (pos < buffer)
			return pos + 1;
	} while (n);

	if (neg)
		*pos-- = '-';
	return pos + 1;
}

void write_int(uint64_t val) {
	const char * str = ltoa(val);
	write(0, str, strlen(str));
}