/* SPDX-License-Identifier: MIT 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Based on nolibc.h, located in the Linux source tree at tools/include/nolibc

*/

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

/* For system call numbers */
#include <linux/unistd.h>

/* For mmap system calls */
#include <linux/mman.h>

/* For fcntl? */
#include <linux/fcntl.h>

/* For reboot system call enum */
#include <linux/reboot.h>

/* For io_uring system calls */
#include <linux/io_uring.h>

/* For clock_gettime */
#include <linux/time.h>

/* For clone3 and other scheduling related things */
#include <linux/sched.h>

/* For futex system call */
#include <linux/futex.h>

/* For openat2 system call */
#include <linux/openat2.h>

/* For system call error values, negated */
#include <linux/errno.h>

/* For signalfd system call */
#include <linux/signalfd.h>

/* For waitid */
#include <linux/wait.h>

/* For struct rusage used by waitid */
#include <linux/resource.h>

/* For ARG_MAX */
#include <linux/limits.h>

/* For sysinfo */
#include <linux/sysinfo.h>

/* uid_t? */
#include <linux/types.h>

/* capabilities(7): capget/capset */
#include <linux/capability.h>

/* fstatat(2) */
#include <asm/stat.h>

/* close_range(2) */
#include <linux/close_range.h>

/* For iovec struct used by msghdr sendmmsg */
#include <linux/uio.h>

/* For kernel sa_family_t */
#include <linux/socket.h>

/* Ethernet packet types */
#include <linux/if_ether.h>

/* Ethernet link-layer structs */
#include <linux/if_packet.h>

/* For mmap, always this value on x86_64 */
#define PAGE_SIZE 4096

/* Standard type and constant definitions */
#define NULL ((void*)0)

typedef enum {false = 0, true = 1} bool;

//#define true ((bool)1)
//#define false ((bool)0)

/* stdint types */
typedef unsigned char       uint8_t;
typedef   signed char        int8_t;
typedef unsigned short     uint16_t;
typedef   signed short      int16_t;
typedef unsigned int       uint32_t;
typedef   signed int        int32_t;
typedef unsigned long long uint64_t;
typedef   signed long long  int64_t;
typedef unsigned long        size_t;
typedef   signed long       ssize_t;
typedef unsigned long     uintptr_t;
typedef   signed long      intptr_t;
typedef   signed long     ptrdiff_t;

typedef unsigned long long uintmax_t;
typedef signed   long long intmax_t;

/* For clock_gettime */
typedef int clockid_t;

typedef unsigned int          dev_t;
typedef unsigned long         ino_t;
typedef unsigned long long	ino64_t;
typedef unsigned int         mode_t;
typedef   signed int          pid_t;
typedef unsigned int          uid_t;
typedef unsigned int          gid_t;
typedef unsigned long       nlink_t;
typedef   signed long         off_t;
typedef   signed long long	off64_t;
typedef   signed long     blksize_t;
typedef   signed long      blkcnt_t;
typedef   signed long        time_t;

/* For waitid */
typedef	signed int idtype_t;
typedef signed int id_t;

#include <linux/signal.h>

/* For getdents64 */
typedef enum {
	DT_UNKNOWN	= 0,
	DT_FIFO   	= 1,
	DT_CHR    	= 2,
	DT_DIR    	= 4,
	DT_BLK    	= 6,
	DT_REG    	= 8,
	DT_LNK    	= 10,
	DT_SOCK   	= 12
} dirtype;

struct linux_dirent64 {
	ino64_t        d_ino;   	/* 64-bit inode number */
	off64_t        d_off;   	/* 64-bit offset to next structure */
	unsigned short d_reclen;	/* Size of this dirent */
	unsigned char  d_type;  	/* File type */
	char           d_name[];	/* Filename (null-terminated) */
};

#define SYSCALL_0(num)                                             \
({                                                                   \
	long _ret;                                                       \
	register long _num  asm("rax") = (num);                          \
                                                                     \
	asm volatile (                                                   \
		"syscall\n"                                                  \
		: "=a" (_ret)                                                \
		: "0"(_num)                                                  \
		: "rcx", "r8", "r9", "r10", "r11", "memory", "cc"            \
	);                                                               \
	_ret;                                                            \
})
#define SYSCALL_1(num, arg1)                                       \
({                                                                   \
	long _ret;                                                       \
	register long _num  asm("rax") = (num);                          \
	register long _arg1 asm("rdi") = (long)(arg1);                   \
                                                                     \
	asm volatile (                                                   \
		"syscall\n"                                                  \
		: "=a" (_ret)                                                \
		: "r"(_arg1),                                                \
		  "0"(_num)                                                  \
		: "rcx", "r8", "r9", "r10", "r11", "memory", "cc"            \
	);                                                               \
	_ret;                                                            \
})

#define SYSCALL_2(num, arg1, arg2)                                 \
({                                                                   \
	long _ret;                                                       \
	register long _num  asm("rax") = (num);                          \
	register long _arg1 asm("rdi") = (long)(arg1);                   \
	register long _arg2 asm("rsi") = (long)(arg2);                   \
                                                                     \
	asm volatile (                                                   \
		"syscall\n"                                                  \
		: "=a" (_ret)                                                \
		: "r"(_arg1), "r"(_arg2),                                    \
		  "0"(_num)                                                  \
		: "rcx", "r8", "r9", "r10", "r11", "memory", "cc"            \
	);                                                               \
	_ret;                                                            \
})

#define SYSCALL_3(num, arg1, arg2, arg3)                           \
({                                                                   \
	long _ret;                                                       \
	register long _num  asm("rax") = (num);                          \
	register long _arg1 asm("rdi") = (long)(arg1);                   \
	register long _arg2 asm("rsi") = (long)(arg2);                   \
	register long _arg3 asm("rdx") = (long)(arg3);                   \
									      \
	asm volatile (                                                   \
		"syscall\n"                                                  \
		: "=a" (_ret)                                                \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3),                        \
		  "0"(_num)                                                  \
		: "rcx", "r8", "r9", "r10", "r11", "memory", "cc"            \
	);                                                               \
	_ret;                                                            \
})

#define SYSCALL_4(num, arg1, arg2, arg3, arg4)                     \
({                                                                   \
	long _ret;                                                       \
	register long _num  asm("rax") = (num);                          \
	register long _arg1 asm("rdi") = (long)(arg1);                   \
	register long _arg2 asm("rsi") = (long)(arg2);                   \
	register long _arg3 asm("rdx") = (long)(arg3);                   \
	register long _arg4 asm("r10") = (long)(arg4);                   \
									      \
	asm volatile (                                                   \
		"syscall\n"                                                  \
		: "=a" (_ret), "=r"(_arg4)                                   \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4),            \
		  "0"(_num)                                                  \
		: "rcx", "r8", "r9", "r11", "memory", "cc"                   \
	);                                                               \
	_ret;                                                            \
})

#define SYSCALL_5(num, arg1, arg2, arg3, arg4, arg5)               \
({                                                                   \
	long _ret;                                                       \
	register long _num  asm("rax") = (num);                          \
	register long _arg1 asm("rdi") = (long)(arg1);                   \
	register long _arg2 asm("rsi") = (long)(arg2);                   \
	register long _arg3 asm("rdx") = (long)(arg3);                   \
	register long _arg4 asm("r10") = (long)(arg4);                   \
	register long _arg5 asm("r8")  = (long)(arg5);                   \
									      \
	asm volatile (                                                   \
		"syscall\n"                                                  \
		: "=a" (_ret), "=r"(_arg4), "=r"(_arg5)                      \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5),\
		  "0"(_num)                                                  \
		: "rcx", "r9", "r11", "memory", "cc"                         \
	);                                                               \
	_ret;                                                            \
})

#define SYSCALL_6(num, arg1, arg2, arg3, arg4, arg5, arg6)          \
({                                                                    \
	long _ret;                                                        \
	register long _num  asm("rax") = (num);                           \
	register long _arg1 asm("rdi") = (long)(arg1);                    \
	register long _arg2 asm("rsi") = (long)(arg2);                    \
	register long _arg3 asm("rdx") = (long)(arg3);                    \
	register long _arg4 asm("r10") = (long)(arg4);                    \
	register long _arg5 asm("r8")  = (long)(arg5);                    \
	register long _arg6 asm("r9")  = (long)(arg6);                    \
                                                                      \
	asm volatile (                                                    \
		"syscall\n"                                                   \
		: "=a" (_ret), "=r"(_arg4), "=r"(_arg5)                       \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
		  "r"(_arg6), "0"(_num)                                       \
		: "rcx", "r11", "memory", "cc"                                \
	);                                                                \
	_ret;                                                             \
})

#define LOCAL static inline
 
LOCAL ssize_t write(int fd, const void *buf, size_t count) {
	return SYSCALL_3(__NR_write, fd, buf, count);
}

LOCAL ssize_t read(int fd, void *buf, size_t count) {
	return SYSCALL_3(__NR_read, fd, buf, count);
}

LOCAL ssize_t reboot(int cmd, void * arg) {
	return SYSCALL_4(__NR_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, cmd, arg);
}

LOCAL __attribute__((noreturn))
void reboot_hard(int cmd) {
	SYSCALL_4(__NR_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, cmd, NULL);
	__builtin_unreachable();
}

LOCAL int sched_yield(void) {
	return SYSCALL_0(__NR_sched_yield);
}

LOCAL ssize_t copy_file_range(int fd_in, off64_t *off_in,
						int fd_out, off64_t *off_out,
						size_t len, unsigned int flags) {
	return SYSCALL_6(__NR_copy_file_range, fd_in, off_in, fd_out, off_out, len, flags);
}

LOCAL off_t lseek(int fd, off_t offset, int whence) {
	return SYSCALL_3(__NR_lseek, fd, offset, whence);
}

LOCAL int umask(mode_t mask) {
	return SYSCALL_1(__NR_umask, mask);
}

LOCAL int mkdir(const char *path, mode_t mode) {
	return SYSCALL_3(__NR_mkdirat, AT_FDCWD, path, mode);
}

LOCAL long mknod(const char *path, mode_t mode, dev_t dev) {
	return SYSCALL_4(__NR_mknodat, AT_FDCWD, path, mode, dev);
}

LOCAL int mount(const char *src, const char *tgt, const char *fst,
	      unsigned long flags, const void *data) {
	return SYSCALL_5(__NR_mount, src, tgt, fst, flags, data);
}

LOCAL int open(const char *path, int flags, mode_t mode) {
	return SYSCALL_4(__NR_openat, AT_FDCWD, path, flags, mode);
}

LOCAL void * brk(void *addr) {
	return (void *)SYSCALL_1(__NR_brk, addr);
}

LOCAL __attribute__((noreturn)) 
void exit(unsigned short status) {
	SYSCALL_1(__NR_exit, (int)status /*status & 255*/);
	__builtin_unreachable();
}

LOCAL int chdir(const char *path) {
	return SYSCALL_1(__NR_chdir, path);
}

LOCAL int chmod(const char *path, mode_t mode) {
	return SYSCALL_4(__NR_fchmodat, AT_FDCWD, path, mode, 0);
}

LOCAL int chown(const char *path, uid_t owner, gid_t group) {
	return SYSCALL_5(__NR_fchownat, AT_FDCWD, path, owner, group, 0);
}

LOCAL int chroot(const char *path) {
	return SYSCALL_1(__NR_chroot, path);
}

LOCAL int close(int fd) {
	return SYSCALL_1(__NR_close, fd);
}

LOCAL int close_range(unsigned int first, unsigned int last, unsigned int flags) {
	return SYSCALL_3(__NR_close_range, first, last, flags);
}

LOCAL int close_all_files() {
	return close_range(0, ~0U, 0);
}

LOCAL int dup3(int oldfd, int newfd, int flags) {
	return SYSCALL_3(__NR_dup3, oldfd, newfd, flags);
}

LOCAL int execveat(int dirfd, const char * filename, char * const argv[], 
		char * const envp[], int flags) {
	return SYSCALL_5(__NR_execveat, dirfd, filename, argv, envp, flags);
}

LOCAL int getdents64(int fd, struct linux_dirent64 *dirp, int count) {
	return SYSCALL_3(__NR_getdents64, fd, dirp, count);
}

LOCAL pid_t getpid(void) {
	return SYSCALL_0(__NR_getpid);
}

LOCAL int ioctl(int fd, unsigned long req, void *value) {
	return SYSCALL_3(__NR_ioctl, fd, req, value);
}

LOCAL int memfd_create(unsigned int flags) {
	return SYSCALL_2(__NR_memfd_create, "",  flags);
}

LOCAL intptr_t mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
	return SYSCALL_6(__NR_mmap, addr, length, prot, flags, fd, offset);
}

LOCAL int munmap(void * addr, size_t length) {
	return SYSCALL_2(__NR_munmap, addr, length);
}

LOCAL intptr_t mremap(void *old_address, size_t old_size, size_t new_size, int flags) {
	/* TODO implement new_address (is it stored in a register?) */
	return SYSCALL_4(__NR_mremap, old_address, old_size, new_size, flags);
}

LOCAL int mprotect(void *addr, size_t len, int prot) {
	return SYSCALL_3(__NR_mprotect, addr, len, prot);
}

LOCAL int io_uring_setup(unsigned entries, struct io_uring_params *p) {
    return SYSCALL_2(__NR_io_uring_setup, entries, p);
}

LOCAL int io_uring_enter(int ring_fd, uint32_t to_submit,
    uint32_t min_complete, uint32_t flags, sigset_t *sig) {
    return SYSCALL_6(__NR_io_uring_enter, ring_fd, to_submit, min_complete,
        flags, sig, 0);
}

LOCAL int io_uring_register(unsigned int fd, unsigned int opcode, void *arg, 
	unsigned int nr_args) {
	return SYSCALL_4(__NR_io_uring_register, fd, opcode, arg, nr_args);
}

LOCAL pid_t clone3(struct clone_args *cl_args, size_t size) {
	return SYSCALL_2(__NR_clone3, cl_args, size);
}

LOCAL int clock_gettime(clockid_t clockid, struct timespec *tp) {
	return SYSCALL_2(__NR_clock_gettime, clockid, tp);
}

/* TODO this system call should be separated into multiple, depending on op */
LOCAL long futex(uint32_t *uaddr, int futex_op, uint32_t val,
	const struct timespec *timeout, /* or: uint32_t val2 */
	uint32_t *uaddr2, uint32_t val3) {
	return SYSCALL_6(__NR_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

LOCAL int openat2(int dirfd, const char *pathname, 
					struct open_how *how, size_t size) {
	return SYSCALL_4(__NR_openat2, dirfd, pathname, how, size);
}

LOCAL int setuid(uid_t uid) {
	return SYSCALL_1(__NR_setuid, uid);
}

LOCAL int setresuid(uid_t ruid, uid_t euid, uid_t suid) {
	return SYSCALL_3(__NR_setresuid, ruid, euid, suid);
}

LOCAL int setresgid(gid_t rgid, gid_t egid, gid_t sgid) {
	return SYSCALL_3(__NR_setresgid, rgid, egid, sgid);
}

LOCAL int setgroups(size_t size, const gid_t *list) {
	return SYSCALL_2(__NR_setgroups, size, list);
}

LOCAL int signalfd(int fd, const sigset_t *mask, int flags) {
	return SYSCALL_4(__NR_signalfd4, fd, mask, sizeof(sigset_t), flags);
}

LOCAL int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
	return SYSCALL_4(__NR_rt_sigprocmask, how, set, oldset, sizeof(sigset_t));
}

LOCAL int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options, 
	struct rusage * usage) {
	return SYSCALL_5(__NR_waitid, idtype, id, infop, options, usage);
}

LOCAL int pivot_root(const char * new_root, const char * put_old) {
	return SYSCALL_2(__NR_pivot_root, new_root, put_old);
}

LOCAL int sysinfo(struct sysinfo * info) {
	return SYSCALL_1(__NR_sysinfo, info);
}

LOCAL int fstatat(int dirfd, const char *restrict pathname,
			struct stat *restrict statbuf, int flags) {
	return SYSCALL_4(__NR_newfstatat, dirfd, pathname, statbuf, flags);
}

LOCAL int prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5) {
	return SYSCALL_5(__NR_prctl, option, arg2, arg3, arg4, arg5);
}

LOCAL int capget(cap_user_header_t hdrp, cap_user_data_t datap) {
	return SYSCALL_2(__NR_capget, hdrp, datap);
}

LOCAL int capset(cap_user_header_t hdrp, cap_user_data_t datap) {
	return SYSCALL_2(__NR_capset, hdrp, datap);
}

LOCAL int setcap(__u32 effective, __u32 permitted, __u32 inheritable) {
	struct __user_cap_header_struct hdr = {
		.version = _LINUX_CAPABILITY_VERSION_3,
		.pid = 0
	};

	struct __user_cap_data_struct data = {
		.effective = effective,
		.permitted = permitted,
		.inheritable = inheritable
	};

	return SYSCALL_2(__NR_capset, &hdr, &data);
}

/* Supported address families. */
#define AF_UNSPEC 0
#define AF_UNIX 1	   /* Unix domain sockets 		*/
#define AF_LOCAL 1	   /* POSIX name for AF_UNIX	*/
#define AF_INET 2	   /* Internet IP Protocol 	*/
#define AF_AX25 3	   /* Amateur Radio AX.25 		*/
#define AF_IPX 4	   /* Novell IPX 			*/
#define AF_APPLETALK 5 /* AppleTalk DDP 		*/
#define AF_NETROM 6	   /* Amateur Radio NET/ROM 	*/
#define AF_BRIDGE 7	   /* Multiprotocol bridge 	*/
#define AF_ATMPVC 8	   /* ATM PVCs			*/
#define AF_X25 9	   /* Reserved for X.25 project 	*/
#define AF_INET6 10	   /* IP version 6			*/
#define AF_ROSE 11	   /* Amateur Radio X.25 PLP	*/
#define AF_DECnet 12   /* Reserved for DECnet project	*/
#define AF_NETBEUI 13  /* Reserved for 802.2LLC project*/
#define AF_SECURITY 14 /* Security callback pseudo AF */
#define AF_KEY 15	   /* PF_KEY key management API */
#define AF_NETLINK 16
#define AF_ROUTE AF_NETLINK /* Alias to emulate 4.4BSD */
#define AF_PACKET 17		/* Packet family		*/
#define AF_ASH 18			/* Ash				*/
#define AF_ECONET 19		/* Acorn Econet			*/
#define AF_ATMSVC 20		/* ATM SVCs			*/
#define AF_RDS 21			/* RDS sockets 			*/
#define AF_SNA 22			/* Linux SNA Project (nutters!) */
#define AF_IRDA 23			/* IRDA sockets			*/
#define AF_PPPOX 24			/* PPPoX sockets		*/
#define AF_WANPIPE 25		/* Wanpipe API Sockets */
#define AF_LLC 26			/* Linux LLC			*/
#define AF_IB 27			/* Native InfiniBand address	*/
#define AF_MPLS 28			/* MPLS */
#define AF_CAN 29			/* Controller Area Network      */
#define AF_TIPC 30			/* TIPC sockets			*/
#define AF_BLUETOOTH 31		/* Bluetooth sockets 		*/
#define AF_IUCV 32			/* IUCV sockets			*/
#define AF_RXRPC 33			/* RxRPC sockets 		*/
#define AF_ISDN 34			/* mISDN sockets 		*/
#define AF_PHONET 35		/* Phonet sockets		*/
#define AF_IEEE802154 36	/* IEEE802154 sockets		*/
#define AF_CAIF 37			/* CAIF sockets			*/
#define AF_ALG 38			/* Algorithm sockets		*/
#define AF_NFC 39			/* NFC sockets			*/
#define AF_VSOCK 40			/* vSockets			*/
#define AF_KCM 41			/* Kernel Connection Multiplexor*/
#define AF_QIPCRTR 42		/* Qualcomm IPC Router          */
#define AF_SMC 43			/* smc sockets: reserve number for \
							 * PF_SMC protocol family that     \
							 * reuses AF_INET address family   \
							 */
#define AF_XDP 44			/* XDP sockets			*/
#define AF_MCTP 45			/* Management component \
							 * transport protocol   \
							 */

#define AF_MAX 46 /* For now.. */

enum sock_type {
	SOCK_STREAM = 1,
	SOCK_DGRAM = 2,
	SOCK_RAW = 3,
	SOCK_RDM = 4,
	SOCK_SEQPACKET = 5,
	SOCK_DCCP = 6,
	SOCK_PACKET = 10,
};

typedef __kernel_sa_family_t sa_family_t;
typedef __u32 socklen_t;

struct msghdr {
	void *msg_name;		   /* Optional address */
	socklen_t msg_namelen; /* Size of address */
	struct iovec *msg_iov; /* Scatter/gather array */
	size_t msg_iovlen;	   /* # elements in msg_iov */
	void *msg_control;	   /* Ancillary data, see below */
	size_t msg_controllen; /* Ancillary data buffer len */
	int msg_flags;		   /* Flags (unused) */
};

struct mmsghdr {
	struct msghdr msg_hdr; /* Message header */
	unsigned int msg_len;  /* Number of bytes transmitted */
};

LOCAL int socket(int domain, int type, int protocol) {
	return SYSCALL_3(__NR_socket, domain, type, protocol);
}

LOCAL int sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen, int flags) {
	return SYSCALL_4(__NR_sendmmsg, sockfd, msgvec, vlen, flags);
}

#endif