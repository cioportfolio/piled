import numpy as np
# import ctypes
import numpy.ctypeslib as ctl
import time
import subprocess

rowcount = 15
colcount = 15
channels = 1
leds = rowcount * colcount

proc = subprocess.Popen([
		"./ledapi",
		"-c", str(channels),
		"-l", str(leds/channels)
	],
	stdin=subprocess.PIPE)

def display(rgb):
	rgb[1::2] = rgb[1::2,::-1] # reverse every other row for "snake" layout matrix
	print("Write: " + str(proc.stdin.write(rgb)))
	proc.stdin.flush()

for it in range(0,10):
	for col in range(0,colcount):

		rgb = np.zeros((rowcount, colcount, 3), dtype=np.uint8)  # three 8 bit color


		rgb[:,col] = np.array([15-col, it,col])
		display(rgb)
		time.sleep(.1) # At 100 fps can handle around 250 leds per channel

proc.terminate()
print(proc.wait())
print("======================== DONE =========================")
