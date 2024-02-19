## Acknowledgements

Adapted from [Lean2 blog](https://iosoft.blog/category/neopixel/)  and [Lutz Koepke's repository](https://gitlab.rlp.net/koepke/upgrade-model)

## Summary

Drive multiple strings (up to 16) of WS2812B LEDs (thousands in total) in parallel on a Raspberry Pi using the secondary memory interface. A simple Python module is used to pipe colour data in a Numpy array to a low level subprocess written in C.

## Quick start

Compile the C subprocess

```bash
gcc -Wall -o ledapi ledapi.c rpi-master/rpi_dma_utils.c -lm
```

Connect the LED data wire to GPIO8

Run demo code (coloured animated pattern on a single string, 15 x 15 matix). Sudo required as the subprocess must run with admin permissions.

```bash
sudo python3 ./demo.py
```

## User Guide

*Note:* When run with sudo python3 will have a different context e.g. different path for packages etc. Either set the PYTHONPATH appropriately or use ```sudo pip``` to install required packages

```python
import piled # Import the led module
import numpy # Optional but recommended for processing byte arrays

piled.start(number_of_LED_strings, number_of_LEDs_in_longest_string) # starts low level subprocess and sets up communications

# Populate a byte array of rgb values. For example:
led_data = numpy.zeros((number_of_rows, number_of_columns, 3), dtype=numpy.uint8) # Create blank of bytes to store rgb values for a matrix of LEDs

piled.display(led_data, debug = False) # Send rgb data to LEDs, optionally display some debug information

piled.stop(debug = False) # Gracefully sutdown low level subprocess and optionally display some debug information
```

## Wiring

This code uses a special hardware interface that works on specific pins (SD0 to SD15) as shown in this diagram

![SD Pins](https://iosoftblog.files.wordpress.com/2020/07/rpi_smi_pinout.png)
