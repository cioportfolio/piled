import numpy as np
import ctypes
import numpy.ctypeslib as ctl
import time

rowcount = 1
colcount = 15
channels = 1
leds = rowcount * colcount

dmalib=ctypes.CDLL('/home/pi/led/LED_DMA.so')
loadleds=dmalib.LED_DMA
# c program expects:
#    pointer  to 8 bit uint array
#    number of LEDs (int)
loadleds.argtypes = [ctl.ndpointer(np.uint8,flags='aligned,c_contiguous'),ctypes.c_int, ctypes.c_int]

# ================== set up array for c program depending on what to display ================
for it in range(0,10):
	for col in range(0,colcount):

		largearray = np.zeros((rowcount, colcount, 3), dtype=np.uint8)  # three 8 bit color


		largearray[0,:] = np.array([colcount-col, 0,col])

		loadleds(largearray, leds, channels)         # call c program
		print(col)
		time.sleep(.1)
largearray = np.zeros((rowcount, colcount, 3), dtype=np.uint8)  # three 8 bit color
loadleds(largearray, leds, channels)         # call c program

print("======================== DONE =========================")
