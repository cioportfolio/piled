import subprocess
import signal

used_chan = 1
used_led = 1
proc = None

def preexec_function():
	signal.signal(signal.SIGINT, signal.SIG_IGN)

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
	stdin=subprocess.PIPE,
	preexec_fn = preexec_function)

def display(rgb, debug = False):
	message = str(proc.stdin.write(rgb))
	if debug:
		print("No of bytes: " + message)
	proc.stdin.flush()

def stop(debug=False):
	proc.terminate()
	message = proc.wait()
	if debug:
		print("Terminating: " + str(message))
