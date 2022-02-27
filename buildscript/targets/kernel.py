from datetime import datetime

from pytz import timezone
from util.target import Target, requires, dirs, is_newer
import util.util
import util.places

from pathlib import Path, PurePosixPath
import shutil

def config_unset(value : str) -> Target:	
	util.util.cmd(
		util.places.kernel_source.joinpath('scripts', 'config'),
		'--file', util.places.installed_config,
		'--disable', value
	)

def kernel_target(target : str):
	util.util.cmd('make', '-C', util.places.kernel_source, '--jobs=4', target)

def kernel_target_str(target : str) -> str:
	return util.util.cmd_str('make', '--silent', '-C', 
		util.places.kernel_source, '--jobs=4', target)

@requires()
def use_saved_config():
	shutil.copy2(util.places.custom_config, util.places.installed_config)

@requires(dirs(util.places.output))
def generate_signing_key():
	if is_newer(util.places.custom_kernel_signing, util.places.output_kernel_signing_key):
		util.util.cmd('openssl', 'req', '-new', '-utf8', '-sha256', 
			'-days', '36500', '-batch', '-x509', 
			'-config', str(util.places.custom_kernel_signing),
			'-outform', 'PEM',
			'-keyout', str(util.places.output_kernel_signing_key),
			'-out', str(util.places.output_kernel_signing_key)
		)

@requires(dirs(util.places.kernel_source))
def build_kernel_initial():
	config_unset('CONFIG_BOOT_CONFIG')
	config_unset('CONFIG_BLK_DEV_INITRD')
	kernel_target('vmlinux')

@requires()
def generate_initramfs():
	'''Generate list of directives for files to include in built-in tmpfs (includes init program)'''
	initramfs = str('''
		# Init program, for some reason this only works if it is named /init
		file /init ../projects/init/init 700 0 0
		#file /modload ../../modload/modload 700 0 0
		#file /hello ../../minlinux/hello 700 0 0

		dir /proc 700 0 0
		dir /sys 700 0 0

		dir /dev 700 0 0
		nod /dev/console 600 0 0 c 5 1	
		''')

	#TODO build initramfs

	Path(util.places.output_kernel_initramfs) \
		.write_bytes(initramfs.encode(encoding='UTF-8'))

@requires(
	use_saved_config, 
	generate_signing_key, 
	build_kernel_initial,
	generate_initramfs,
	)
def build():
	'''Build the linux kernel'''
	kernel_target('bzImage')
	pass

@requires()
def clean():
	shutil.rmtree(util.places.output)
	kernel_target('clean')

@requires()
def use_updated_config():
	'''Copy config from kernel source directory to git tracked directory'''
	shutil.copy2(util.places.installed_config, util.places.custom_config)
	print('Updated config at ' + str(util.places.custom_config) + ' ' + 
		str(datetime.fromtimestamp(Path(util.places.custom_config).stat().st_mtime, timezone('UTC'))))

@requires(build)
def run_efi():
	util.util.cmd(
		'qemu-system-x86_64', '-vga', 'none', 
		'-nographic', '-sandbox', 'on',
		'-kernel', util.places.kernel_source.joinpath(kernel_target_str('image_name').strip()),
		'-append', 'console=ttyS0',
		'-bios', '/usr/share/ovmf/OVMF.fd'
	)