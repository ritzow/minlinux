from pathlib import PurePosixPath

Posix = PurePosixPath

project_root: Posix = Posix(__file__).parent.parent.parent
kernel_project: Posix = project_root.joinpath('kernel')
kernel_source: Posix = project_root.joinpath('linux')
custom_config : Posix = kernel_project.joinpath('.config')
custom_kernel_signing : Posix = kernel_project.joinpath('signing.conf')
installed_config: Posix = kernel_source.joinpath('.config')

output: Posix = project_root.joinpath('build')
output_kernel_signing_key: Posix = output.joinpath('kernel_key.pem')
output_kernel_initramfs: Posix = output.joinpath('initramfs.conf')