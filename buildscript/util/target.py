from pathlib import Path, PosixPath, PurePosixPath
from typing import Callable, Dict, Tuple

Target = Callable[[], None]
calledTargets = []
availableTargets: Dict[str, Tuple[Target, Target]] = dict()

def target_name(func : Target):
	return func.__module__ + '.' + func.__name__

def target_description(t : Target):
	return t.__name__ +  ((': ' + t.__doc__) if t.__doc__ != None else '')

def is_any_newer(dst : PurePosixPath, *src : PurePosixPath) -> bool:
	dstfile = PosixPath(dst)
	if not dstfile.exists():
		return True
	dstmtime = dstfile.stat().st_mtime
	for file in src:
		srcmtime = PosixPath(file).stat().st_mtime
		if srcmtime > dstmtime:
			return True
	return False

def is_newer(src : PurePosixPath, dst : PurePosixPath) -> bool:
	dstfile = PosixPath(dst)
	return not dstfile.exists() or \
		(PosixPath(src).stat().st_mtime > dstfile.stat().st_mtime)

def requires(*depends : Target):
	def transform(func : Target): #-> Callable[[Callable[[], None]], Callable[[], None]]:
		def on_target():
			# Using func as the identity works because it is pass-by-refernce
			# It refers to the member of the module, not the specific code
			if func not in calledTargets:
				for target in depends:
					target()
				func()
				calledTargets.append(func)
		availableTargets[func.__name__] = (func, on_target)
		return on_target
	return transform

def dirs(*depends : PurePosixPath) -> Target:
	def make_dirs(): #Target instance
		'''Generate directories: ''' + str.join(', ', (str(x) for x in depends))
		for dir in depends:
			Path(dir).mkdir(parents=True, exist_ok=True)
	return make_dirs