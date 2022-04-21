
from util.target import requires
import util.places
import subprocess
import shutil

@requires()
def dropbear():
	'Compiles the dropbear SSH server'
	subprocess.run([
		str(util.places.dropbear_configure),
		'--prefix=' + str(util.places.output_dropbear), 
		'--enable-static',
		'--enable-bundled-libtom',
		'--disable-shadow',
		'--disable-openpty',
		'--disable-lastlog',
		'--disable-utmp',
		'--disable-utmpx',
		'--disable-wtmp',
		'--disable-wtmpx',
		'--disable-loginfunc',
		'--disable-pututline',
		'--disable-pututxline',
		'--disable-syslog',
		'--disable-zlib',
		'CC=' + str(util.places.musl_gcc)
	], cwd=util.places.dropbear_source)
	shutil.copy2(util.places.dropbear_config, util.places.output_dropbear_config)
	subprocess.run([
		shutil.which('make'),
		'-C', str(util.places.dropbear_source),
		'PROGRAMS=dropbear',
		'strip', 'install'
	], cwd=util.places.dropbear_source)
