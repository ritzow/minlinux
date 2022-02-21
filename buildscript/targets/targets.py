from buildscript.target import *

from . import kernel as kern, project

@requires(kern.build)
def kernel():
	'''Build the linux kernel'''
	print("kernel")
	pass