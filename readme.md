Raspberry Pi source files, see https://iosoft.blog  and https://gitlab.rlp.net/koepke/upgrade-model for details

C version

```bash
rpi-master$ gcc -Wall -o plas32 rpi_plasma32.c rpi_dma_utils.c -lm

rpi-master$ sudo ./plas32 -n 128 -t
```

Python wrap experiment

```bash
gcc -shared -Wl,-soname,LED_DMA.so -o LED_DMA.so -fPIC LED_DMA.c rpi-master/rpi_dma_utils.c

sudo python3 ./LED_DMA.py
```
