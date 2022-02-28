from util.target import requires

@requires()
def lib_util():
	pass

@requires(lib_util)
def init():
	'''Build the /init program'''
	pass