import numpy as np

import numpy.ctypeslib as ctl
import subprocess

used_chan = 1
used_led = 1
proc = None

def start(channels, ledsperchannel):
	global used_chan
	global used_led
	global proc

	used_chan = channels
	used_led = ledsperchannel

	proc = subprocess.Popen([
		"./ledapi",
		"-c", str(used_chan),
		"-l", str(used_led)
	],
	stdin=subprocess.PIPE)

def display(rgb, debug = False):
	rgb[1::2] = rgb[1::2,::-1] # reverse every other row for "snake" layout matrix
	message = str(proc.stdin.write(rgb))
	if debug:
		print("No of bytes: " + message)
	proc.stdin.flush()

def stop(debug=False):
	proc.terminate()
	message = proc.wait()
	if debug:
		print("Terminating: " + str(message))