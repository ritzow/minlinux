#!/usr/bin/env -S python3 -B

import util.target
#Not used directly but import is required
import targets.targets
assert targets.targets
import sys

def execute_target(target : util.target.Target):
	target()
	# print("Called:")
	# for t in util.target.calledTargets:
	# 	print(" - " + util.target.target_description(t))

def main():
	@util.target.requires()
	def help():
		'''List available targets'''
		print('Available targets (' + str(len(util.target.availableTargets)) + '):')
		for _, pair in sorted(util.target.availableTargets.items()):
			func, _ = pair
			print(' - ' + util.target.target_description(func))
	for name in sys.argv[1:]:
		target = util.target.availableTargets.get(name)
		if target is not None:
			func, actual = target
			execute_target(actual)
		else:
			print("Target '" + name + "' not found")
	if len(sys.argv) == 1:
		print('Usage: ' + sys.argv[0] + ' <target>...')
		help()

if __name__ == '__main__':
	main()