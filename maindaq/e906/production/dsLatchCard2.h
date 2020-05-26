/******************************************************************\
*                                                                  *
*  dsLatchCard.h  - Header for Da-Shung's Latch Card VME           *
*                   I/O Register Library                           *
*                                                                  *
*  Author: David Abbott                                            *
*          Jefferson Lab Data Acquisition Group                    *
*          April 2003                                              *
*                                                                  *
*  dsLatchCard.h - Header for Da-Shung's Latch Card                *
*             Multi Event Latch Library                            *
*                                                                  *
*  Author: Shiu Shiuan Hal                                         *
*          May 2009                                                *
*          Grass Wang					 	   *
*          May 2010						   *  
*          Jia-Ye Chen					 	   *
*          July 2010						   *  
*                                                                  *
* This header file is modified from the dsLatchCard.h              * 
\******************************************************************/

/* --- Define Structure for access to Local Memory map --- */

struct latch2_struct{
  volatile unsigned int csr;             // 0x000  Control and Status Register
  volatile unsigned int id;              // 0x004  PCB number and Revision date code
  volatile unsigned int fifoStatusReg;   // 0x008  FIFO status register
  volatile unsigned int csr2;            // 0x00C  Control and Status Register 2
  volatile unsigned int fifoTestEnable;  // 0x010  FIFO test mode enable
  volatile unsigned int fifoTestDisable; // 0x014  FIFO test mode disable
  volatile unsigned int blank1[2];       // 0x018  <->  0x01C
  volatile unsigned int clr;             // 0x020  Clear
  volatile unsigned int blank2;          // 0x024
  volatile unsigned int trigEnable;      // 0x028  Latch mode enable  (trigger enable)
  volatile unsigned int trigDisable;     // 0x02C  Latch mode disable (trigger disable)
  volatile unsigned int blank3[8];       // 0x030  <->  0x04C
  volatile unsigned int reserved1;       // 0x050  Reserved
  volatile unsigned int reserved2;       // 0x054  Reserved
  volatile unsigned int blank4[2];       // 0x058  <->  0x05C
  volatile unsigned int reset;           // 0x060  Reset
  volatile unsigned int blank5;          // 0x064
  volatile unsigned int genOutputPulse;  // 0x068  Generate output pulse
  volatile unsigned int dummy[37];       // dummy
  volatile unsigned int Event[64];       // event
};

struct latch2_data{
  volatile unsigned int data[64];  // 0x100~0x1FC  128KB FIFO memory space, (32Kx32)
};

#define LATCH_MAX_BOARDS       10

/* Define default interrupt vector/level */
#define LATCH_INT_VEC        0xe0
#define LATCH_VME_INT_LEVEL     5

/* Control/Status Register bits */
#define userLED            0x0001  // 0000 0000 0000 0000 0000 0000 0000 0001
#define eclOutput1         0x0002  // 0000 0000 0000 0000 0000 0000 0000 0010
#define eclOutput2         0x0004  // 0000 0000 0000 0000 0000 0000 0000 0100
#define eclOutput3         0x0008  // 0000 0000 0000 0000 0000 0000 0000 1000

/* -- FIFO Status Mode = 00 -- */
#define fifoStatusMode1    0x0000  // 0000 0000 0000 0000 0000 0000 0000 0000
/* -- FIFO Status Mode = 01 -- */
#define fifoStatusMode2    0x0010  // 0000 0000 0000 0000 0000 0000 0001 0000
/* -- FIFO Status Mode = 10 -- */
#define fifoStatusMode3    0x0020  // 0000 0000 0000 0000 0000 0000 0010 0000
/* -- FIFO Status Mode = 11 -- */
#define fifoStatusMode4    0x0030  // 0000 0000 0000 0000 0000 0000 0011 0000

#define eclInput1          0x0040  // 0000 0000 0000 0000 0000 0000 0100 0000
#define eclInput2          0x0080  // 0000 0000 0000 0000 0000 0000 1000 0000
#define fifoTestMode       0x0100  // 0000 0000 0000 0000 0000 0001 0000 0000
#define captureMode        0x0200  // 0000 0000 0000 0000 0000 0010 0000 0000
#define reserved1          0x0400  // 0000 0000 0000 0000 0000 0100 0000 0000
#define reserved2          0x0800  // 0000 0000 0000 0000 0000 1000 0000 0000
/*-- Latch_mode_SEL (0x0000:strobe mode) -- */
#define onEdgeMode1        0x1000  // 0000 0000 0000 0000 0001 0000 0000 0000
#define onEdgeMode2        0x1010  // 0000 0000 0000 0000 0001 0000 0000 0000
#define onEdgeMode3        0x1020  // 0000 0000 0000 0000 0001 0000 0000 0000
#define onEdgeMode4        0x1030  // 0000 0000 0000 0000 0001 0000 0000 0000
#define reserved3          0x2000  // 0000 0000 0000 0000 0010 0000 0000 0000
#define reserved4          0x4000  // 0000 0000 0000 0000 0100 0000 0000 0000
#define reserved5          0x8000  // 0000 0000 0000 0000 1000 0000 0000 0000

/* FIFO status mask */
#define fifoStatusModeMask 0xffffffcf
#define fifoStatusMask 0xffff
/* -- CSR2 OR combination -- */
#define or1ON1             0x0001  // 0000 0000 0000 0001
#define or1ON2             0x0002  // 0000 0000 0000 0010
#define or1ON3             0x0004  // 0000 0000 0000 0100
#define or1ON4             0x0008  // 0000 0000 0000 1000
#define or1ON5             0x0010  // 0000 0000 0001 0000
#define or1ON6             0x0020  // 0000 0000 0010 0000

#define or2ON1             0x0003  // 0000 0000 0000 0011
#define or2ON2             0x0006  // 0000 0000 0000 0110
#define or2ON3             0x000c  // 0000 0000 0000 1100
#define or2ON4             0x0018  // 0000 0000 0001 1000
#define or2ON5             0x0030  // 0000 0000 0011 0000

#define or3ON1             0x0007  // 0000 0000 0000 0111
#define or3ON2             0x000e  // 0000 0000 0000 1110
#define or3ON3             0x001c  // 0000 0000 0001 1100
#define or3ON4             0x0038  // 0000 0000 0011 1000

#define or4ON1             0x000f  // 0000 0000 0000 1111
#define or4ON2             0x001e  // 0000 0000 0001 1110
#define or4ON3             0x003c  // 0000 0000 0011 1100

#define or5ON1             0x001f  // 0000 0000 0001 1111
#define or5ON2             0x003e  // 0000 0000 0011 1110

#define or6ON              0x003f  // 0000 0000 0011 1111
