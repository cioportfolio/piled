import numpy as np
import piled
import time

rowcount = 15
colcount = 15
channels = 1
leds = rowcount * colcount


def blankLEDs():
    # three 8 bit color
    return np.zeros((rowcount, colcount, 3), dtype=np.uint8)


piled.start(channels, leds/channels)
for it in range(0, 10):
    for col in range(0, colcount):
        rgb = blankLEDs()
        rgb[:, col] = np.array([15-col, it, col])
        rgb[1::2] = rgb[1::2,::-1] # reverse every other row for "snake" layout matrix

        piled.display(rgb)
        time.sleep(.01)  # At 100 fps can handle around 250 leds per channel

rgb = blankLEDs()
piled.display(rgb)
time.sleep(.1) # Allow time for the last blank frame to be sent
piled.stop()
print("======================== DONE =========================")
