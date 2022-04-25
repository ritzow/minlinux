import os
import subprocess

from pytz import timezone
from util.target import requires, dirs, is_newer
import util.places

from pathlib import Path, PosixPath, PurePosixPath
import shutil

def config_modify(*args : str):
	subprocess.run([
		util.places.kernel_source.joinpath('scripts', 'config'),
		'--file', util.places.installed_config,
		*args
	])

def config_enable(*values : str):
	for val in values:
		config_modify('--enable', val)

def config_disable(*values : str):	
	for val in values:
		config_modify('--disable', val)

def config_set_str(option : str, value : str):
	config_modify('--set-str', option, value)

def config_set_val(option : str, value : str):
	config_modify('--set-val', option, value)

def kernel_target(target : str):
	subprocess.run([shutil.which('make'), '-C', 
		util.places.kernel_source, '--jobs=4', target])

def kernel_target_str(target : str) -> str:
	return subprocess.run([shutil.which('make'), '--silent', 
		'-C', util.places.kernel_source, 
		'--jobs=4', target], capture_output=True).stdout.decode(encoding='UTF-8')

@requires()
def kernel_help():
	subprocess.run([shutil.which('make'), '--silent', '-C', 
		util.places.kernel_source, 'help'])

@requires()
def use_saved_config():
	'''Reset the config installed in the linux source directory'''
	shutil.copy2(util.places.custom_config, util.places.installed_config)

@requires(dirs(util.places.output))
def signing_key():
	'''Generate a private key for signing kernel modules, stored in the linux kernel'''
	if is_newer(util.places.custom_kernel_signing, 
			util.places.output_kernel_signing_key):
		subprocess.run([shutil.which('openssl'), 'req', '-new', '-utf8', '-sha256', 
			'-days', '36500', '-batch', '-x509', 
			'-config', str(util.places.custom_kernel_signing),
			'-outform', 'PEM',
			'-keyout', str(util.places.output_kernel_signing_key),
			'-out', str(util.places.output_kernel_signing_key)
		])

def lines(*strs) -> str:
	return '\n'.join(list(strs))

@requires()
def generate_initramfs():
	'''Generate list of directives for files to include in 
	built-in tmpfs (includes init program)'''
	if is_newer(PurePosixPath(__file__), util.places.output_kernel_initramfs):
		initramfs = lines(
			#this only works if it is named /init (required by linux initramfs)
			'file /init ' + str(util.places.output_init_elf) + ' 777 0 0',
			'file /hello ' + str(util.places.output_bin.joinpath('hello')) + ' 777 0 0',
			'file /busybox /home/ritzow/busybox 777 0 0',
			'file /dropbear ' + str(util.places.dropbear_elf) + ' 777 0 0',
			'dir /proc 777 0 0',
			'dir /sys 777 0 0',
			'dir /dev 777 0 0',
		)
		PosixPath(util.places.output_kernel_initramfs) \
			.write_bytes(initramfs.encode(encoding='UTF-8'))

#TODO build system should allow each target to provide a timestamp or
#indicate if outdated, then all dependent targets will be called again

@requires()
def bare_kernel():
	use_saved_config()
	kernel_target('vmlinux')

@requires(bare_kernel, dirs(util.places.output))
def kernel_headers():
	'''Install the Linux kernel headers in the output directory for use by init'''
	new_env = os.environ.copy()
	new_env.update([('INSTALL_HDR_PATH', str(util.places.output))])
	subprocess.Popen([shutil.which('make'), '-C', 
		util.places.kernel_source, 'headers_install',
		'INSTALL_HDR_PATH=' + str(util.places.output)], 
		executable=shutil.which('make'),
		env=new_env).wait()

# TODO enable CONFIG_IP_PNP for automatic DHCP!!!

@requires(
	signing_key,
	generate_initramfs,
	kernel_headers
)
def kernel():
	'''Build the bootable linux kernel and init system'''
	use_saved_config()
	config_enable('CONFIG_BLK_DEV_INITRD')
	config_set_val('INITRAMFS_ROOT_UID', '0')
	config_set_val('INITRAMFS_ROOT_GID', '0')
	config_enable('INITRAMFS_COMPRESSION_NONE')
	config_disable('ACPI_TABLE_UPGRADE')
	config_set_str('CONFIG_INITRAMFS_SOURCE', util.places.output_kernel_initramfs)
	config_disable(
		'CONFIG_RD_GZIP',
		'CONFIG_RD_BZIP2',
		'CONFIG_RD_LZMA',
		'CONFIG_RD_XZ',
		'CONFIG_RD_LZO',
		'CONFIG_RD_ZSTD',
		'CONFIG_RD_LZ4'
	)
	kernel_target('bzImage')

@requires()
def clean():
	'''Delete all build output for kernel and file system'''
	shutil.rmtree(util.places.output, ignore_errors=True)
	kernel_target('clean')

@requires()
def save_config():
	'''Copy config from kernel source directory to git tracked directory'''
	shutil.copy2(util.places.installed_config, util.places.custom_config)
	from datetime import datetime
	timestamp = datetime.fromtimestamp(
		Path(util.places.custom_config).stat().st_mtime, timezone('UTC'))
	print('Updated config at ' + str(util.places.custom_config) + ' ' + str(timestamp))

def kernel_path() -> PurePosixPath:
	return  util.places.kernel_source.joinpath(
			kernel_target_str('image_name').strip())

@requires()
def run_efi():
	'''Build and run the kernel image in the QEMU emulator as an EFI application'''
	subprocess.run([
		shutil.which('qemu-system-x86_64'), '-vga', 'none', 
		'-nographic', '-sandbox', 'on',
		'-kernel', str(kernel_path()),
		'-device', 'e1000,netdev=net0',
		'-netdev', 'user,id=net0,hostfwd=tcp::5555-:22',
		'-append', 'console=ttyS0', #does rdinit= work to set the /init path
		'-bios', '/usr/share/ovmf/OVMF.fd'
	])

@requires()
def kernel_info():
	print('Kernel size: ' + str(PosixPath(kernel_path()).stat().st_size) + ' bytes')

@requires()
def diff_configs():
	'''Diff the saved kernel config and the installed kernel config. Installed in green'''
	subprocess.run([shutil.which('diff'), '--color=always', '--speed-large-files',
		'-s', util.places.custom_config, util.places.installed_config]) 
		#| less -r --quit-if-one-screen --quit-at-eof)

@requires(use_saved_config)
def configure():
	'''Open the Linux kernel menuconfig to edit the installed kernel config'''
	kernel_target("menuconfig")
	save_config()