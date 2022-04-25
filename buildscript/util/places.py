from pathlib import PurePosixPath

Posix = PurePosixPath

# Kernel and dsystem

project_root: Posix = Posix(__file__).parent.parent.parent
kernel_project: Posix = project_root.joinpath('kernel')
kernel_source: Posix = project_root.joinpath('linux')
custom_config: Posix = kernel_project.joinpath('.config')
custom_kernel_signing: Posix = kernel_project.joinpath('signing.conf')
installed_config: Posix = kernel_source.joinpath('.config')

includes: Posix = project_root.joinpath('include')

output: Posix = project_root.joinpath('build')
output_kernel_signing_key: Posix = output.joinpath('kernel_key.pem')
output_kernel_initramfs: Posix = output.joinpath('initramfs.conf')
output_bin: Posix = output.joinpath('bin')

output_includes: Posix = output.joinpath('include')
output_manual: Posix = output.joinpath('manual')

output_init: Posix = output_bin.joinpath('init')
output_init_elf: Posix = output_init.joinpath('init')

# Dropbear

dropbear_source: Posix = project_root.joinpath('dropbear')
dropbear_config: Posix = kernel_project.joinpath('dropbear_options.h')
dropbear_configure: Posix = dropbear_source.joinpath('configure')
output_dropbear: Posix = output_bin.joinpath('dropbear')
output_dropbear_config: Posix = dropbear_source.joinpath('localoptions.h')
dropbear_elf: Posix = output_dropbear.joinpath('sbin').joinpath('dropbear')

# musl

musl_source: Posix = project_root.joinpath('musl')
musl_configure: Posix = musl_source.joinpath('configure')
output_musl: Posix = output_bin.joinpath('musl')
musl_gcc: Posix = output_musl.joinpath('bin').joinpath('musl-gcc')