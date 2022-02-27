from . import kernel
from util.target import requires

@requires(kernel.build)
def kernel():
	'''Build the linux kernel'''
	pass