
#List all target files here
from pathlib import PosixPath
import subprocess, shutil
from util.target import dirs, requires
import util.places
from . import kernel, project, dropbear, musl
assert kernel, project

@requires(dirs(util.places.output_manual))
def manual():
	#if not PosixPath(util.places.output_manual).exists():
	subprocess.run([shutil.which('make'), 
		'-C', 'man-pages', 
		'-j', 'install', 
		'prefix=' + str(util.places.output_manual)])
	try:
		manpage = input('Manual page: ')
		subprocess.run([shutil.which('man'), '--manpath=' + 
			str(util.places.output_manual.joinpath('share', 'man')), manpage])
	except KeyboardInterrupt:
		print()
#	$(MAKE) -C man-pages -j install prefix=$(BUILD_DIR)/manual