from typing import Callable

Target = Callable[[], None]

calledTargets = []

availableTargets = set()

def target_name(func : Target):
	return func.__module__ + '.' + func.__name__

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
		availableTargets.add(func)
		return on_target
	return transform