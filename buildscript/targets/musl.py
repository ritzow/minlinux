from util.target import requires
import util.places
import subprocess
import shutil

@requires()
def musl():
	'Compiles musl libc'
	subprocess.run([
		str(util.places.musl_configure),
		'--prefix=' + str(util.places.output_musl), 
		'--disable-shared',
	], cwd=util.places.musl_source)
	subprocess.run([
		shutil.which('make'),
		'-C', str(util.places.musl_source),
		'all', 'install'
	], cwd=util.places.musl_source)