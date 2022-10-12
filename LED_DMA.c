// Raspberry Pi WS2812 LED driver using SMI
// For detailed description, see https://iosoft.blog
//
// Copyright (c) 2020 Jeremy P Bentham
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// v0.01 JPB 16/7/20 Adapted from rpi_smi_adc_test v0.06
// v0.02 JPB 15/9/20 Addded RGB to GRB conversion
// v0.03 JPB 15/9/20 Added red-green flashing
// v0.04 JPB 16/9/20 Added test mode
// v0.05 JPB 19/9/20 Changed test mode colours
// v0.06 JPB 20/9/20 Outlined command-line data input
// v0.07 JPB 25/9/20 Command-line data input if not in test mode
// v0.08 JPB 26/9/20 Changed from 4 to 3 pulses per LED bit
//                   Added 4-bit zero preamble
//                   Added raw Tx data test
// v0.09 JPB 27/9/20 Added 16-channel option
// v0.10 JPB 28/9/20 Corrected Pi Zero caching problem
// v0.11 JPB 29/9/20 Added enable_dma before transfer (in case still active)
//                   Corrected DMA nsamp value (was byte count)
// v0.12 JPB 26/5/21 Corrected transfer length for 16-bit mode
//
// modified by LHK 05/12/21 simplified to accept buffer prepared by python code
// may want to switch to DMA channel 4 or 5, because these are not lite and are twice as wide
//
// Further modified by RJB Sep 22 to drive 32x32 matrix from python program
//

#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "rpi-master/rpi_dma_utils.h"
#include "rpi-master/rpi_smi_defs.h"

#if PHYS_REG_BASE==PI_4_REG_BASE        // Timings for RPi v4 (1.5 GHz)
#define SMI_TIMING       10, 15, 30, 15    // 400 ns cycle time
#else                                   // Timings for RPi v0-3 (1 GHz)
#define SMI_TIMING       10, 10, 20, 10   // 400 ns cycle time (original)
//#define SMI_TIMING       4, 26, 52, 26   // 416 ns cycle time
//#define SMI_TIMING       10, 12, 16, 12   // 400 ns cycle time
#endif

#define LED_D0_PIN      8   // GPIO pin for D0 output
#define LED_NCHANS      8   // Number of LED channels (8 or 16)
#define LED_NBITS       24  // Number of data bits per LED (3 colors with 8 bits each)
#define LED_PREBITS     4   // Number of zero bits before LED data (default 4)
#define LED_POSTBITS    4   // Number of zero bits after LED data
#define BIT_NPULSES     3   // Number of O/P pulses per LED bit
#define CHAN_MAXLEDS    128 // Maximum number of LEDs per channel
#define REQUEST_THRESH  2   // DMA request threshold
#define DMA_CHAN        5  // DMA channel to use (may want to change to  5, to get more than 65533 byte transfers)

// Length of data for 1 row (1 LED on each channel)
#define LED_DLEN        (LED_NBITS * BIT_NPULSES)

// Transmit data type, 8 or 16 bits
#if LED_NCHANS > 8
#define TXDATA_T        uint16_t
#else
#define TXDATA_T        uint8_t
#endif

// Structures for mapped I/O devices, and non-volatile memory
extern MEM_MAP gpio_regs, dma_regs, clk_regs;
MEM_MAP vc_mem, smi_regs;

// Pointers to SMI registers
volatile SMI_CS_REG  *smi_cs;
volatile SMI_L_REG   *smi_l;
volatile SMI_A_REG   *smi_a;
volatile SMI_D_REG   *smi_d;
volatile SMI_DMC_REG *smi_dmc;
volatile SMI_DSR_REG *smi_dsr;
volatile SMI_DSW_REG *smi_dsw;
volatile SMI_DCS_REG *smi_dcs;
volatile SMI_DCA_REG *smi_dca;
volatile SMI_DCD_REG *smi_dcd;

// Ofset into Tx data buffer, given LED number in chan
#define LED_TX_OSET(n)      (LED_PREBITS + (LED_DLEN * (n)))

// Size of data buffers & NV memory, given number of LEDs per chan
#define TX_BUFF_LEN(n)      (LED_TX_OSET(n) + LED_POSTBITS)
#define TX_BUFF_SIZE(n)     (TX_BUFF_LEN(n) * sizeof(TXDATA_T))
#define VC_MEM_SIZE         (PAGE_SIZE + TX_BUFF_SIZE(CHAN_MAXLEDS))


TXDATA_T *txdata;                               // Pointer to uncached Tx data buffer
TXDATA_T tx_buffer[TX_BUFF_LEN(CHAN_MAXLEDS)];  // Tx buffer for assembling data

void swap_bytes(void *data, int len);
void mat_txdata(uint8_t *leds, int n_LEDs, int n_CHANs, TXDATA_T *txd);
void map_devices(void);
void fail(char *s);
void terminate(int sig);
void init_smi(int width, int ns, int setup, int hold, int strobe);
void setup_smi_dma(MEM_MAP *mp, int nsamp);
void start_smi(MEM_MAP *mp);

//----------------------------------------------------------------------------
int LED_DMA(uint8_t* array, int n_LEDs, int n_CHANs)
//----------------------------------------------------------------------------
{
    signal(SIGINT, terminate);                  //<----- what is this for?

    map_devices();
    init_smi(LED_NCHANS>8 ? SMI_16_BITS : SMI_8_BITS, SMI_TIMING);
    map_uncached_mem(&vc_mem, VC_MEM_SIZE);
    setup_smi_dma(&vc_mem, TX_BUFF_LEN(CHAN_MAXLEDS));
    mat_txdata(array, n_LEDs, n_CHANs, &tx_buffer[LED_TX_OSET(0)]);
#if LED_NCHANS <= 8
    swap_bytes(tx_buffer, TX_BUFF_SIZE(CHAN_MAXLEDS));
#endif
    memcpy(txdata, tx_buffer, TX_BUFF_SIZE(CHAN_MAXLEDS));
    enable_dma(DMA_CHAN);
    start_smi(&vc_mem);
    usleep(10);
    while (dma_active(DMA_CHAN))
    	usleep(10);
    terminate(0);
    return(0);
}

// Swap adjacent bytes in transmit data
void swap_bytes(void *data, int len)
{
    uint16_t *wp = (uint16_t *)data;

    len = (len + 1) / 2;
    while (len-- > 0)
    {
        *wp = __builtin_bswap16(*wp);
        wp++;
    }
}


//----------------------------------------------------------------------------
// Map GPIO, DMA and SMI registers into virtual mem (user space)
// If any of these fail, program will be terminated
void map_devices(void)
//----------------------------------------------------------------------------
{
    map_periph(&gpio_regs, (void *)GPIO_BASE, PAGE_SIZE);
    map_periph(&dma_regs,  (void *)DMA_BASE,  PAGE_SIZE);
    map_periph(&clk_regs,  (void *)CLK_BASE,  PAGE_SIZE);
    map_periph(&smi_regs,  (void *)SMI_BASE,  PAGE_SIZE);
}

// Catastrophic failure in initial setup
void fail(char *s)
{
    printf(s);
    terminate(0);
}
//----------------------------------------------------------------------------
// Free memory segments and exit
void terminate(int sig)
//----------------------------------------------------------------------------
{
    int i;

    printf("Closing\n");
    if (gpio_regs.virt)
    {
        for (i=0; i<LED_NCHANS; i++)
            gpio_mode(LED_D0_PIN+i, GPIO_IN);
    }
    if (smi_regs.virt)
        *REG32(smi_regs, SMI_CS) = 0;
    stop_dma(DMA_CHAN);
    unmap_periph_mem(&vc_mem);
    unmap_periph_mem(&smi_regs);
    unmap_periph_mem(&dma_regs);
    unmap_periph_mem(&gpio_regs);
    //exit(0);
    return;
}
//----------------------------------------------------------------------------
// Initialise SMI, given data width, time step, and setup/hold/strobe counts
// Step value is in nanoseconds: even numbers, 2 to 30
void init_smi(int width, int ns, int setup, int strobe, int hold)
//----------------------------------------------------------------------------
{
    int i, divi = ns / 2;

    smi_cs  = (SMI_CS_REG *) REG32(smi_regs, SMI_CS);
    smi_l   = (SMI_L_REG *)  REG32(smi_regs, SMI_L);
    smi_a   = (SMI_A_REG *)  REG32(smi_regs, SMI_A);
    smi_d   = (SMI_D_REG *)  REG32(smi_regs, SMI_D);
    smi_dmc = (SMI_DMC_REG *)REG32(smi_regs, SMI_DMC);
    smi_dsr = (SMI_DSR_REG *)REG32(smi_regs, SMI_DSR0);
    smi_dsw = (SMI_DSW_REG *)REG32(smi_regs, SMI_DSW0);
    smi_dcs = (SMI_DCS_REG *)REG32(smi_regs, SMI_DCS);
    smi_dca = (SMI_DCA_REG *)REG32(smi_regs, SMI_DCA);
    smi_dcd = (SMI_DCD_REG *)REG32(smi_regs, SMI_DCD);
    smi_cs->value = smi_l->value = smi_a->value = 0;
    smi_dsr->value = smi_dsw->value = smi_dcs->value = smi_dca->value = 0;
    if (*REG32(clk_regs, CLK_SMI_DIV) != divi << 12)
    {
        *REG32(clk_regs, CLK_SMI_CTL) = CLK_PASSWD | (1 << 5);
        usleep(10);
        while (*REG32(clk_regs, CLK_SMI_CTL) & (1 << 7)) ;
            usleep(10);
        *REG32(clk_regs, CLK_SMI_DIV) = CLK_PASSWD | (divi << 12);
        usleep(10);
        *REG32(clk_regs, CLK_SMI_CTL) = CLK_PASSWD | 6 | (1 << 4);
        usleep(10);
        while ((*REG32(clk_regs, CLK_SMI_CTL) & (1 << 7)) == 0) ;
            usleep(100);
    }
    if (smi_cs->seterr)
        smi_cs->seterr = 1;
    smi_dsr->rsetup    = smi_dsw->wsetup = setup;
    smi_dsr->rstrobe   = smi_dsw->wstrobe = strobe;
    smi_dsr->rhold     = smi_dsw->whold = hold;
    smi_dmc->panicr    = smi_dmc->panicw = 8;
    smi_dmc->reqr      = smi_dmc->reqw = REQUEST_THRESH;
    smi_dsr->rwidth    = smi_dsw->wwidth = width;
    for (i=0; i<LED_NCHANS; i++)
        gpio_mode(LED_D0_PIN+i, GPIO_ALT1);
}
//----------------------------------------------------------------------------
// Set up SMI transfers using DMA
void setup_smi_dma(MEM_MAP *mp, int nsamp)
//----------------------------------------------------------------------------
{
    DMA_CB *cbs=mp->virt;

    txdata = (TXDATA_T *)(cbs+1);
    smi_dmc->dmaen = 1;
    smi_cs->enable = 1;
    smi_cs->clear  = 1;
    smi_cs->pxldat = 1;
    smi_l->len = nsamp * sizeof(TXDATA_T);
    smi_cs->write  = 1;
    enable_dma(DMA_CHAN);
    cbs[0].ti = DMA_DEST_DREQ | (DMA_SMI_DREQ << 16) | DMA_CB_SRCE_INC | DMA_WAIT_RESP;
    cbs[0].tfr_len = nsamp * sizeof(TXDATA_T);
    cbs[0].srce_ad = MEM_BUS_ADDR(mp, txdata);
    cbs[0].dest_ad = REG_BUS_ADDR(smi_regs, SMI_D);
}
//----------------------------------------------------------------------------
// Start SMI DMA transfers
void start_smi(MEM_MAP *mp)
//----------------------------------------------------------------------------
{
    DMA_CB *cbs=mp->virt;

    start_dma(mp, DMA_CHAN, &cbs[0], 0);
    smi_cs->start = 1;
}

void mat_txdata(uint8_t *leds, int n_LEDs, int n_CHANs, TXDATA_T *txd)
{
    int i, n, l, o;
    uint8_t by, bi,msk, cr,cg,cb;

    for (l=0; l<n_LEDs/n_CHANs; l++)
    {    
        // For each bit of the 24-bit RGB values..
        for (n=0; n<LED_NBITS; n++)
        {
            // Mask to convert RGB to GRB, M.S bit first
            msk = n==0 ? 0x80 : n==8 ? 0x80 : n==16 ? 0x80 : msk>>1;
            // 1st byte or word is a high pulse on all lines
            txd[0] = (TXDATA_T)0xffff;
            // 2nd has high or low bits from data
            // 3rd is a low pulse
            txd[1] = txd[2] = 0;
            for (i=0; i<LED_NCHANS; i++)
            {
		if (i < n_CHANs)
		{
		        o = (i*CHAN_MAXLEDS + l)*3;
		        cr = leds[o];
		        cg = leds[o+1];
		        cb = leds[o+2];
		        bi = n/8;
		        if (bi == 0)
		        {
		            by = cg;
		        }
		        else
		        {
		            if (bi == 1)
		            {
		                by = cr;
		            }
		            else
		            {
		                by = cb;
		            }
		        }
		}
		else
		{
			by = 0;
		}
                if (by & msk)
                    txd[1] |= (1 << i);
            }
	    // printf("LED %i Bit %i: %04x\n", l, n, txd[1]);
            txd += BIT_NPULSES;
        }
    }
}

// EOF
