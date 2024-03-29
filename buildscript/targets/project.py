from pathlib import PosixPath, PurePosixPath
from typing import List
from util.target import is_any_newer, requires
import util.places
import shutil
import subprocess
import itertools

# TODO need to check for header file changes still

def gcc(inputs : List[PurePosixPath], output : PurePosixPath, flags : List[str]):
	import util.log
	util.log.log('Generate \'' + str(output) + '\'')
	args = [shutil.which('gcc'), 
		'-Wall', '-Wextra', '-Werror', '-Wfatal-errors', 
		'-fno-builtin', 
		# Disable standard headers
		'-nostdinc',
		'-Wno-unused-parameter', '-Wno-unused-variable',
		'-fno-asynchronous-unwind-tables', '-fno-ident', 
		'-O3', '-nostdlib', '-static', '-lgcc', '-pie',
		'-I', str(util.places.includes), 
		'-I', str(util.places.output_includes),
		'-march=x86-64',
		'-o', str(output), *flags,
		*[str(f) for f in inputs]]
	if subprocess.run(args, stderr=subprocess.STDOUT).returncode != 0:
		raise RuntimeError

def out_file(input : PurePosixPath) -> PurePosixPath:
	rel = input.relative_to(util.places.project_root).with_suffix('.o')
	return PosixPath(util.places.output_bin, rel)

def gcc_c(input : PurePosixPath):
	outfile = out_file(input)
	outfile.parent.mkdir(parents=True, exist_ok=True)
	if is_any_newer(outfile, input, PurePosixPath(__file__)):
		gcc([input], outfile, ['-c'])

def c_files(*rel_dir : str):
	#if rel_dir.is_absolute():
	#	raise ValueError(rel_dir)
	return PosixPath(util.places.project_root.joinpath(*rel_dir)).glob("**/*.c")

def out_for(*dirs : str):
	return [out_file(f) for f in itertools.chain(*tuple(c_files(dir) for dir in dirs))]

@requires()
def hello_world():
	'''Build test hello world program'''
	for file in c_files('hello-world'):
		gcc_c(file)
	gcc(out_for('hello-world', 'util'), util.places.output_bin.joinpath('hello'), [])

@requires()
def lib_util():
	'''Build the utility library used by /init'''
	for file in c_files('util'):
		gcc_c(file)

@requires()
def lib_init():
	for file in c_files('init'):
		gcc_c(file)

@requires(lib_util, lib_init)
def init():
	'''Build the /init program'''
	dependencies = out_for('init', 'util')
	if is_any_newer(util.places.output_init_elf, *dependencies):
		gcc(dependencies, util.places.output_init_elf, [])