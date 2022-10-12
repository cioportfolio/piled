import numpy as np
# import ctypes
import numpy.ctypeslib as ctl
import time
import subprocess

rowcount = 1
colcount = 15
channels = 1
leds = rowcount * colcount

proc = subprocess.Popen("./ledapi",
stdin=subprocess.PIPE,
stdout=subprocess.PIPE)

# dmalib=ctypes.CDLL('/home/pi/led/LED_DMA.so')
# loadleds=dmalib.LED_DMA
# # c program expects:
# #    pointer  to 8 bit uint array
# #    number of LEDs (int)
# loadleds.argtypes = [ctl.ndpointer(np.uint8,flags='aligned,c_contiguous'),ctypes.c_int, ctypes.c_int]

# ================== set up array for c program depending on what to display ================
for it in range(0,1):
	for col in range(0,colcount):

		largearray = np.zeros((rowcount, colcount, 3), dtype=np.uint8)  # three 8 bit color


		largearray[0,:] = np.array([colcount-col, 0,col])

		# loadleds(largearray, leds, channels)         # call c program
		print("Write: " + proc.stdin.write(largearray))
		print("Return message:" + proc.stdout.read(3))
		time.sleep(2)
proc.terminate()
print(proc.wait())
print("======================== DONE =========================")
