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
	stdin=subprocess.PIPE) #,
#stdout=subprocess.PIPE)

#print(proc.stdout.readline(50).decode("utf-8"))

# # c program expects:
# #    8 bit uint array

for it in range(0,10):
	for col in range(0,colcount):

		rgb = np.zeros((rowcount, colcount, 3), dtype=np.uint8)  # three 8 bit color


		rgb[:,col] = np.array([15-col, it,col])

		print("Write: " + str(proc.stdin.write(rgb)))
		proc.stdin.flush()
		time.sleep(.01)

proc.terminate()
print(proc.wait())
print("======================== DONE =========================")
