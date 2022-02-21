#!/usr/bin/env -S python3 -B

import buildscript.target as target_util
import buildscript.targets.targets as global_targets
import sys

if __name__ == '__main__':
	for name in sys.argv[1:]:
		try:
			target = getattr(global_targets, name)
			target()
			print("Called:")
			for t in target_util.calledTargets:
				print(" - " + target_util.target_name(t))
		except AttributeError:
			print("Target '" + name + "' not found")

	if len(sys.argv) == 1:
		print('Usage: ./build.py <target>...')
		print('Available targets (' + str(len(target_util.availableTargets)) + '):')
		for t in sorted(target_util.availableTargets, key=lambda func: func.__name__):
			print(' - ' + t.__name__ +  ((': ' + t.__doc__) if t.__doc__ != None else ''))