# Single-File No-disk, No-libc Linux

This is a set of Makefile helper targets, a Linux kernel config, and a small 
init program which preoduce a Linux bzImage file which can run directly as an 
EFI application without interaction with persistent storage (probably via PXE 
boot). The kernel config is 
very specific and doesn't target a broad range of hardware. It doesn't include 
all the proper drivers or other robust system support. This could serve as a base 
for a node in a server cluster started using a custom orchestration system, 
somewhat similar to running Fedora CoreOS as a simplified Kubernetes node.

Directory sl-src contains the source code for a simple init program that has
no library requirements other than the sanitized kernel headers. 
It uses code snippets from the Linux source
tree at tools/include/nolibc/nolibc.h, for using system calls and some other
libc-like functions. The init program also uses io_uring, currently for a simple
echo program, based on example code from liburing's io_uring(7) manual page.

## Usage

This was sloppily thrown together so documentation is poor and the Makefile is
brittle, and so on.

The Makefile target build-all builds the whole thing (except for installing
prerequisites). Prerequisites can be installed with the apt-* targets. The 
kernel is first compiled without an initramfs to produce 
modules.builtin and other files required to properly install generated kernel 
modules. The enabled kernel modules are then built, followed by a build of
bzImage, which will partially build the kernel again, this time with a built-in 
initramfs (generated from the spec in initramfs.conf and target 'build'). The 
initramfs 
contains /init, which is executed by the kernel on startup (it has to be that 
exact file path for some reason, see: 
https://www.kernel.org/doc/Documentation/filesystems/ramfs-rootfs-initramfs.txt). 
Modules are signed using the key generated from kernel_keygen.conf. The file 
boot.conf is not currently used because bootconfig doesn't seem to compile,
but this would otherwise be used to specify extra kernel arguments and bake them
into bzImage in an efficient way.

A Makefile target 'run' using qemu-system-x86_64 is provided to test the kernel 
from a terminal.