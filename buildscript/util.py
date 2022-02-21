import subprocess

def cmd(*args):
	subprocess.run(list(args))