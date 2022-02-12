# Container tools

runc (standard container runtime)
crun (container runtime as .so)
gdebi (install .deb packages and deps to specific root folder)
skopeo (download stuff from docker)
umoci (convert docker download to OCI bundle config.json+rootfs)
debootstrap (create bootstrap debian installation)
dpkg (installing debian packages, required by multistrap)
multistrap (similar to debootstrap but more configurable?)
dropbear ssh (simpler ssh server)
init_module system call, for loading modules.
pacman, dnf (rpm), emerge (portage) for creating containers and host software.

# Kbuild notes

need to specify kernel cmd line args still
might not need to specify kernel cmd args
specify CONFIG_DEFAULT_INIT to my custom init binary
may need to re-enable CONFIG_AUDIT (system call tracing?)
Investigate  CONFIG_RESET_ATTACK_MITIGATION
CONFIG_MODULES has info for compiling necessary modules
Set CONFIG_MODPROBE_PATH to whever I choose to put modprobe
fresco logic driver needed for usb 3.0 on G74Sx
build kernel modules first, move them to initramfs, generate cpio archive, then build the kernel.

certs/signing_key.pem (and potentially other files?) may need to be copied and used during the second kernel build in order to properly work. 
certs/signing_key.pem will need to be supplied in config during second build. 
This will allow the modules to be signed with the key while working for the second generated kernel as well.

# Container runtime/orchestration
linux host spawns init process: uses unix sockets to communicate with containers, starts by spawning a control-server container.
uses libcrun to create containers with specs provided by container processes using unix sockets to communicate with init.
control-server communicates with control-client.
control-client orchestrates mutiple linux hosts.
control-client will usually be another server in a container, which provides a web GUI to connecting browsers, and a command line interface.

# jemalloc

Seems to require a version of libc for jemalloc.h to work.

See CONFIG_VT_CONSOLE, try disabling and see what happens, maybe even remove /dev/console?

# libkmod

Try compiling statically with musl just from the source code provided in the libkmod directory.
Requires zstd.h for Zstd compression, compile from https://github.com/facebook/zstd/tree/dev/lib

# init

Make folders appear in ls with a slash at the end (what macros to use to determine if folder?)
