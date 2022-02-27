#!/usr/bin/env -S python3 -B

import util.target
import targets.targets
import sys

def execute_target(target : util.target.Target):
	target()
	print("Called:")
	for t in util.target.calledTargets:
		print(" - " + util.target.target_description(t))

if __name__ == '__main__':
	for name in sys.argv[1:]:
		try:
			execute_target(getattr(targets.targets, name))
		except AttributeError:
			for t in util.target.availableTargets:
				if t.__name__ == name:
					execute_target(t)
					sys.exit(None)
			print("Target '" + name + "' not found")

	if len(sys.argv) == 1:
		print('Usage: ./build.py <target>...')
		print('Available targets (' + str(len(util.target.availableTargets)) + '):')
		for t in sorted(util.target.availableTargets, key=lambda func: func.__name__):
			print(' - ' + util.target.target_description(t))