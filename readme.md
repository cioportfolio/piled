Raspberry Pi source files, see https://iosoft.blog  and https://gitlab.rlp.net/koepke/upgrade-model for details

Python pipe between python and subprocess written in C

```bash
gcc -Wall -o ledapi ledapi.c rpi-master/rpi_dma_utils.c -lm

sudo python3 ./LED_DMA.py
```