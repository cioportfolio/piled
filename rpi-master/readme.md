Raspberry Pi source files, see https://iosoft.blog for details

gcc -Wall -o plas32 rpi_plasma32.c rpi_dma_utils.c -lm

sudo ./plas32 -n 128 -t