/**************************************************************************
*
*  sis3610.h  - Header for SIS 3610 VME I/O Register Library
*
*
*  Author: David Abbott 
*          Jefferson Lab Data Acquisition Group
*          April 2003
*
*/


/* Define Structure for access to Local Memory map*/

struct s3610_struct {
  volatile unsigned int csr;
  volatile unsigned int id;
  volatile unsigned int d_out;
  volatile unsigned int jk_out;
  volatile unsigned int d_in;
  volatile unsigned int l_in;
  volatile unsigned int blank[18];
  volatile unsigned int reset;
};

#define S3610_MAX_BOARDS     2
#define S3610_BOARD_ID       0x36100000
#define S3610_BOARD_ID_MASK  0xffff0000
#define S3610_VERSION_MASK   0x0000f000

/* Define default interrupt vector/level */
#define S3610_INT_VEC      0xe0
#define S3610_VME_INT_LEVEL   5

/* Control/Status Register bits */
#define S3610_STATUS_USER_LED            0x1
#define S3610_STATUS_INT_MODE            0x2
#define S3610_STATUS_STROBE_BIT0         0x4
#define S3610_STATUS_STROBE_BIT1         0x8

#define S3610_STATUS_USER_OUT1          0x10
#define S3610_STATUS_USER_OUT2          0x20
#define S3610_STATUS_USER_OUT3          0x40
#define S3610_STATUS_USER_OUT4          0x80
#define S3610_STATUS_FF_ENABLE1      0x10000
#define S3610_STATUS_FF_ENABLE2      0x20000
#define S3610_STATUS_FF_ENABLE3      0x40000
#define S3610_STATUS_FF_ENABLE4      0x80000
#define S3610_STATUS_INT_VME_IRQ   0x4000000
#define S3610_STATUS_VME_IRQ       0x8000000

#define S3610_CNTL_SET_USER_LED              0x1
#define S3610_CNTL_SET_ROAK                  0x2
#define S3610_CNTL_SET_LS0                   0x4
#define S3610_CNTL_SET_LS1                   0x8
#define S3610_CNTL_SET_USER_OUT1            0x10
#define S3610_CNTL_SET_USER_OUT2            0x20
#define S3610_CNTL_SET_USER_OUT3            0x40
#define S3610_CNTL_SET_USER_OUT4            0x80

#define S3610_CNTL_CLR_USER_LED            0x100
#define S3610_CNTL_SET_RORA                0x200
#define S3610_CNTL_CLR_LS0                 0x400
#define S3610_CNTL_CLR_LS1                 0x800
#define S3610_CNTL_CLR_USER_OUT1          0x1000
#define S3610_CNTL_CLR_USER_OUT2          0x2000
#define S3610_CNTL_CLR_USER_OUT3          0x4000
#define S3610_CNTL_CLR_USER_OUT4          0x8000

#define S3610_CNTL_ENABLE_FF1            0x10000
#define S3610_CNTL_ENABLE_FF2            0x20000
#define S3610_CNTL_ENABLE_FF3            0x40000
#define S3610_CNTL_ENABLE_FF4            0x80000
#define S3610_CNTL_ENABLE_IRQ0          0x100000
#define S3610_CNTL_ENABLE_IRQ1          0x200000
#define S3610_CNTL_ENABLE_IRQ2          0x400000
#define S3610_CNTL_ENABLE_IRQ3          0x800000

#define S3610_CNTL_DISABLE_FF1         0x1000000
#define S3610_CNTL_DISABLE_FF2         0x2000000
#define S3610_CNTL_DISABLE_FF3         0x4000000
#define S3610_CNTL_DISABLE_FF4         0x8000000
#define S3610_CNTL_DISABLE_IRQ0       0x10000000
#define S3610_CNTL_DISABLE_IRQ1       0x20000000
#define S3610_CNTL_DISABLE_IRQ2       0x40000000
#define S3610_CNTL_DISABLE_IRQ3       0x80000000

#define S3610_INT_ENABLE              0x00000800

/* Define Bit Masks */
#define S3610_INT_VEC_MASK            0x000000ff
#define S3610_INT_LEVEL_MASK          0x00000700
#define S3610_INT_SOURCE_ENABLE_MASK  0x00f00000
#define S3610_INT_SOURCE_VALID_MASK   0xf0000000
#define S3610_FF_ENABLE_MASK          0x000f0000
#define S3610_USER_OUTPUT_MASK        0x000000f0
#define S3610_LATCH_STROBE_MASK       0x0000000c


/* Define some macros */
#define S3610_IRQ_ENABLE(intID)    s3610p[intID]->id |= S3610_INT_ENABLE
#define S3610_IRQ_DISABLE(intID)   s3610p[intID]->id &= ~S3610_INT_ENABLE
#define S3610_SET_OUTPUT(id, val)  {s3610p[id]->d_out = val;}
#define S3610_SET_JK(id,val)       {s3610p[id]->jk_out = (val&0xffff);}
#define S3610_CLR_JK(id,val)       {s3610p[id]->jk_out = ((val&0xffff)<<16);}
#define S3610_SET_BIT(id, bit)     {s3610p[id]->jk_out = (1<<bit);}

