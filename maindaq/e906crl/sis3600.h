/******************************************************************\
*                                                                  *
*  sis3610.h  - Header for SIS 3610 VME I/O Register Library       *
*                                                                  *
*                                                                  *
*  Author: David Abbott                                            *
*          Jefferson Lab Data Acquisition Group                    *
*          April 2003                                              *
*                                                                  *
*  sis3600.h - Header for SIS 3600 Multi Event Latch Library       *
*                                                                  *
*                                                                  *
*  Author: Shiu Shiuan Hal                                         *
*                                                                  *
*          May 2009                                           *
*                                                                  *
* This header file is modified from the sis3610.h                  * 
\******************************************************************/


/* Define Structure for access to Local Memory map*/



struct s3600_struct {
  volatile unsigned int csr;       /*Control and Status Register(0x000) */
  volatile unsigned int id;        /*module IDentification register and IRQ control register(0x004) */
  volatile unsigned int fc;        /*8-bit Fast Clear window value(0x008) */
  volatile unsigned int ofr;       /*Output pulse Frequency register(0x00C) */
  volatile unsigned int wf;        /*Write to FIFO(in FIFO test mode)(0x010) */
  volatile unsigned int blank1[3]; /*(0x014~0x01c)*/
  volatile unsigned int cf;        /*Clear FIFO and logic(0x020) */
  volatile unsigned int vmenc;     /*VME next clock (0x024) */
  volatile unsigned int ennc;      /*Enable next clock logic(0x028) */
  volatile unsigned int dinc;      /*Disable next clock logic(0x02c) */
  volatile unsigned int bcf;       /*Broadcast,Clear FIFO and logic(0x030) */
  volatile unsigned int bvmenc;    /*Broadcast,VME next clock (0x034) */
  volatile unsigned int bennc;     /*Broadcast,Enable next clock logic(0x038) */
  volatile unsigned int bdinc;     /*Broadcast,Disable next clock logic(0x03c) */
  volatile unsigned int blank2[4]; /*(0x040~0x04C)*/
  volatile unsigned int enfc;      /*Enable fast clear(0x050)*/
  volatile unsigned int difc;      /*Disable fast clear(0x054)*/
  volatile unsigned int blank3[2]; /*(0x058~0x05C)*/
  volatile unsigned int reset;     /*Reset register(0x060)*/
  volatile unsigned int blank4;    /*(0x064)*/
  volatile unsigned int geop;      /*Generate one output pulse(0x068)*/

};

struct s3600_data {
  /*volatile unsigned int data1;  (0x100~0x1FC)*/
  /*  volatile unsigned int data[128];  (0x100~0x1FC)*/
  volatile unsigned int data[64];  /*(0x100~0x1FC)*/
};

/*************************************************/

#define S3600_MAX_BOARDS     2
#define S3600_BOARD_ID       0x36000000
#define S3600_BOARD_ID_MASK  0xffff0000
#define S3600_VERSION_MASK   0x0000f000

/* Define default interrupt vector/level */
#define S3600_INT_VEC      0xe0
#define S3600_VME_INT_LEVEL   5

/*************************************************/



/* Control/Status Register bits */
#define S3600_STATUS_USER_LED             0x1
#define S3600_STATUS_FIFO_MODE            0x2
#define S3600_STATUS_OUTPUT0              0x4
#define S3600_STATUS_OUTPUT1              0x8
#define S3600_STATUS_EN_OUTPUT           0x10
#define S3600_STATUS_PIPE_MODE           0x20
#define S3600_STATUS_BROAD_MODE          0x40
#define S3600_STATUS_HAND          0x80
#define S3600_STATUS_FF_EMPTY           0x100
#define S3600_STATUS_FF_ALEMPTY         0x200
#define S3600_STATUS_FF_HALFFULL        0x400
#define S3600_STATUS_FF_AFULL           0x800
#define S3600_STATUS_FF_FULL           0x1000

#define S3600_STATUS_FAST_CLEAR        0x4000
#define S3600_STATUS_EN_NECLOCK        0x8000
#define S3600_STATUS_EN_EXTNEXT       0x10000
#define S3600_STATUS_EN_EXTCLEAR      0x20000
#define S3600_STATUS_LATCH_GATE       0x40000
#define S3600_STATUS_COIN_MODE        0x80000
#define S3600_STATUS_VME_IRQEN0      0x100000
#define S3600_STATUS_VME_IRQEN1      0x200000
#define S3600_STATUS_VME_IRQEN2      0x400000
#define S3600_STATUS_VME_IRQEN3      0x800000


#define S3600_STATUS_INT_VME_IRQ    0x4000000
#define S3600_STATUS_VME_IRQ        0x8000000
#define S3600_STATUS_VME_IRQS1     0x10000000
#define S3600_STATUS_VME_IRQS2     0x20000000
#define S3600_STATUS_VME_IRQS3     0x40000000
#define S3600_STATUS_VME_IRQS4     0x80000000

#define S3600_CNTL_SET_USER_LED              0x1
#define S3600_CNTL_SET_FIFO                  0x2
#define S3600_CNTL_SET_OUTPUT0               0x4
#define S3600_CNTL_SET_OUTPUT1               0x8
#define S3600_CNTL_EN_OUTPUT                0x10
#define S3600_CNTL_EN_PIPE_MODE             0x20
#define S3600_CNTL_EN_BROAD_MODE            0x40
#define S3600_CNTL_EN_HAND                  0x80

#define S3600_CNTL_CLR_USER_LED            0x100
#define S3600_CNTL_DIS_FIFOTEST            0x200
#define S3600_CNTL_CLR_OUTPUT0             0x400
#define S3600_CNTL_CLR_OUTPUT1             0x800
#define S3600_CNTL_DIS_OUTPUT             0x1000
#define S3600_CNTL_DIS_PIPELINE           0x2000
#define S3600_CNTL_DIS_BROADCAST          0x4000
#define S3600_CNTL_DIS_HAND               0x8000

#define S3600_CNTL_ENABLE_EXNEXT         0x10000
#define S3600_CNTL_ENABLE_EXCLEAR        0x20000
#define S3600_CNTL_SET_LATCHGATE         0x40000
#define S3600_CNTL_SET_COINMODE          0x80000
#define S3600_CNTL_ENABLE_IRQ0          0x100000
#define S3600_CNTL_ENABLE_IRQ1          0x200000
#define S3600_CNTL_ENABLE_IRQ2          0x400000
#define S3600_CNTL_ENABLE_IRQ3          0x800000

#define S3600_CNTL_DISABLE_EXNEXT      0x1000000
#define S3600_CNTL_DISABLE_EXCLEAR     0x2000000
#define S3600_CNTL_CLEAR_LATCH         0x4000000
#define S3600_CNTL_CLEAR_COINMODE      0x8000000
#define S3600_CNTL_DISABLE_IRQ0       0x10000000
#define S3600_CNTL_DISABLE_IRQ1       0x20000000
#define S3600_CNTL_DISABLE_IRQ2       0x40000000
#define S3600_CNTL_DISABLE_IRQ3       0x80000000

#define S3600_INT_ENABLE              0x00000800



/* Define Bit Masks */
#define S3600_INT_VEC_MASK            0x000000ff
#define S3600_INT_LEVEL_MASK          0x00000700
#define S3600_INT_SOURCE_ENABLE_MASK  0x00f00000
#define S3600_INT_SOURCE_VALID_MASK   0xf0000000
#define S3600_FF_ENABLE_MASK          0x000f0000
#define S3600_USER_OUTPUT_MASK        0x000000f0
#define S3600_Output_Mode_MASK       0x0000000c






/* Define some macros */
#define S3610_IRQ_ENABLE(intID)    s3610p[intID]->id |= S3610_INT_ENABLE
#define S3610_IRQ_DISABLE(intID)   s3610p[intID]->id &= ~S3610_INT_ENABLE
#define S3610_SET_OUTPUT(id, val)  {s3610p[id]->d_out = val;}
#define S3610_SET_JK(id,val)       {s3610p[id]->jk_out = (val&0xffff);}
#define S3610_CLR_JK(id,val)       {s3610p[id]->jk_out = ((val&0xffff)<<16);}
#define S3610_SET_BIT(id, bit)     {s3610p[id]->jk_out = (1<<bit);}

