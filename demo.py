import numpy as np
import piled
import time

rowcount = 8
colcount = 15
channels = 8
leds = rowcount * colcount


def blankLEDs():
    # three 8 bit color
    return np.zeros((rowcount, colcount, 3), dtype=np.uint8)


piled.start(channels, leds/channels)
try:
    while True:
        for it in range(20):
            for col in range(colcount):
                rgb = blankLEDs()
                for ch in range(channels):
                    rgb[ch,it%5] = np.array([colcount-col, it, col])
                    rgb[ch, ch%5] = np.array([16,16,16])
#                rgb[1::2] = rgb[1::2,::-1] # reverse every other row for "snake" layout matrix
                piled.display(rgb)
                time.sleep(.05)  # At 100 fps can handle around 250 leds per channel
except KeyboardInterrupt:
    print("Interrupt")
finally:
    rgb = blankLEDs()
    piled.display(rgb)
    time.sleep(.1) # Allow time for the last blank frame to be sent
    piled.stop()
    print("======================== DONE =========================")
