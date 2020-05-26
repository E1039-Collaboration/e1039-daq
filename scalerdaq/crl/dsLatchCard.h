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

struct latch_struct{
  volatile unsigned int csr;             // 0x000  Control and Status Register
  volatile unsigned int id;              // 0x004  PCB number and Revision date code
  volatile unsigned int fifoStatusReg;   // 0x008  FIFO status register
  volatile unsigned int outputPulseReg;  // 0x00C  Output pulse register
  volatile unsigned int fifoTestEnable;  // 0x010  FIFO test mode enable
  volatile unsigned int fifoTestDisable; // 0x014  FIFO test mode disable
  volatile unsigned int blank1[2];       // 0x018  <->  0x01C
  volatile unsigned int clr;             // 0x020  Clear
  volatile unsigned int blank2;          // 0x024
  volatile unsigned int trigEnable;      // 0x028  Trigger Enable (Capture mode enable)
  volatile unsigned int trigDisable;     // 0x02C  Trigger Disable (Capture mode disable)
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

struct latch_data{
  volatile unsigned int data[64];  // 0x100~0x1FC  128KB FIFO memory space, (32Kx32)
};

#define LATCH_MAX_BOARDS       10

/* Define default interrupt vector/level */
#define LATCH_INT_VEC        0xe0
#define LATCH_VME_INT_LEVEL     5

/* Control/Status Register bits */
#define userLED            0x0001
#define eclOutput1         0x0002
#define eclOutput2         0x0004
#define eclOutput3         0x0008

#define fifoStatusMode1    0x0000  // -- FIFO Status Mode = 00
#define fifoStatusMode2    0x0010  // -- FIFO Status Mode = 01
#define fifoStatusMode3    0x0020  // -- FIFO Status Mode = 10
#define fifoStatusMode4    0x0030  // -- FIFO Status Mode = 11

#define eclInput1          0x0040
#define eclInput2          0x0080
#define fifoTestMode       0x0100
#define onStrobeMode       0x0200
#define reserved1          0x0400
#define reserved2          0x0800
#define onEdgeMode         0x1000  // -- edge trigger mode (0x0000:strobe mode)
#define reserved3          0x2000
#define reserved4          0x4000
#define reserved5          0x8000

/* FIFO status mask */
#define fifoStatusModeMask 0xffffffcf
