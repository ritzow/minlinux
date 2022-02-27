import subprocess
import sys

def cmd(*args):
	subprocess.run(list(args))

def cmd_str(*args) -> str:
	return subprocess.run(list(args), capture_output=True).stdout.decode(encoding='UTF-8')